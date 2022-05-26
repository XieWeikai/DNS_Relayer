////
//// Created by 谢卫凯 on 2022/3/31.
////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "wrap.h"

#include "thread_pool.h"
#include "message.h"
#include "cache.h"
#include "log.h"

#define DEBUG 0

#define STR_LEN 64
#define MAX_NUM 10

#define DNS_PORT 53

typedef struct {
//    message *msg;
    char buff[1024]; // 接收到的请求报文
    size_t n;  // 请求报文的长度
    struct sockaddr_in cliAddr;
} arg;

// 以下这个结构存入缓存
// key为name:type
// 如www.baidu.com:A这个key中存放着的item的type为A ipv4顾名思义，DDL是过期时间，strVal当type为CNAME NS之类的时候才用
// ipv4中存的是网络字节序
typedef struct {
    char name[STR_LEN]; // 该资源属于哪个name
    uint8_t type; // 记录类型
    uint8_t num;  // 记录个数，最多MAX_NUM个
    uint32_t ipv4[MAX_NUM];
    char strVal[MAX_NUM][STR_LEN + 1];
    time_t ddl; // 过期时间
} item;

// 依据name type 返回key key存入res中，成功则返回true,失败返回false
// 如调用calcKey("www.baidu.com",CNAME,buf)
// 调用后buf中存的字符串为 "www.baidu.com:CNAME"
// 调用方要确保res 足够长
int calcKey(char *name, int type, char *res) {
    static char *typename[30] = {NULL};
    typename[A] = "A";
    typename[AAAA] = "AAAA";
    typename[CNAME] = "CNAME";
    typename[NS] = "NS";
    if (type <= 0 || type >= 30 || typename[type] == NULL) // 无效的type
        return false;
    res[0] = 0;
    strcat(res, name);
    strcat(res, ":");
    strcat(res, typename[type]);
    return true;
}

int globalSocket;
Cache *globalCache;

// 将item中存的资源记录填入msg的answer段中
// 该函数为辅助函数
// msg为待设置的报文 it为缓存中记录的一项数据 name为RR对应的name
void Item2Msg(message *msg, item *it, char *name) {
    RR rr;
    bzero(&rr, sizeof(rr));
    setRRName(&rr,name);
    rr.type = it->type;
    rr.class = IN;
    rr.TTL = it->ddl - time(NULL);
    if (it->type == A) {
        if(it->ipv4[0] == 0){// 非法域名屏蔽 ！！！！
            setRCODE(msg,NAME_ERR);
            return;
        }

        for (int i = 0; i < it->num; i++) {
            setRRData(&rr,&(it->ipv4[i]), sizeof(uint32_t));
            addRR(msg, &rr, ANSWER);
        }
    }else{
        for (int i = 0; i < it->num; i++) {
            setRRNameData(&rr, it->strVal[i]);
            addRR(msg, &rr, ANSWER);
        }
    }
}

// 将rr中的记录放入it中
// 注意it中存放的类型要与rr的类型一致
void addRR2Item(item *it,RR *rr){
    it->type = rr->type;
    strncpy(it->name,rr->name,STR_LEN);
    it->ddl = rr->TTL + time(NULL);
    if(it->type == A)
        memcpy(&(it->ipv4[it->num++]),rr->data, sizeof(uint32_t));
    else
        strncpy(it->strVal[it->num++],rr->string_data,STR_LEN);
}

// 取出 name:type中的缓存
item *getItem(char *name,int type){
    char key[51];
    calcKey(name,type,key);
    return CacheGet(globalCache,key);
}

// 将一个条目放入缓存中
void putItem(char *name,int type,item *it){
    char key[51];
    if(it->type == 0) // 无效的item
        return ;
    calcKey(name,type,key);
    CachePut(globalCache, key, it, sizeof(item), it->ddl - time(NULL));
}

// debug用的 看看item里面装的什么
void checkItem(item *it){
    static char *typename[6];
    char str[20];
    struct in_addr ad;

    typename[A] = "A";
    typename[NS] = "NS";
    typename[CNAME] = "CNAME";
    log_debug("item type:%s",typename[it->type]);
    log_debug("item name:%s",it->name);
    log_debug("item TTL:%d",it->ddl-time(NULL));
    log_debug("num:%d",it->num);
    for(int i=0;i<it->num;i++){
        if(it->type == A) {
            ad.s_addr = it->ipv4[i];
            log_debug("ipv4:%s", inet_ntop(AF_INET, &ad, str, 20));
        }else
            log_debug("data:%s",it->strVal[i]);
    }
}

// 展示一下请求报文中问题部分的内容
void showRequest(message *msg){
    static char *typename[50] = {NULL};
    typename[A] = "A";
    typename[CNAME] = "CNAME";
    typename[NS] = "NS";
    log_info("request for:%s type:%s",msg->ques[0]->q_name,typename[msg->ques[0]->q_type]);
}

void tmpSend(void *buff,size_t n); // debug函数

void handler(void *ar) {
    char buff[1024];
    int n, needSearch = false;
    arg *parg = ar;
    message *replyMsg = decode(parg->buff); // 要回复的msg
    releaseAdditionalRR(replyMsg); //把额外的段删去，不知为啥dig的请求中有时会带有additional段
    showRequest(replyMsg);

#if DEBUG
    log_debug("-----request-----");
    showMsg(replyMsg);
    log_debug("-----end of request------");
#endif

    char name[STR_LEN];
    strncpy(name,replyMsg->ques[0]->q_name,STR_LEN); // name放着将查询的域名字符串
    uint16_t qtype = replyMsg->ques[0]->q_type; //qtype中存放着将查询的资源类型

    // 下面尝试从缓存中读取资源记录
    item *it,*tit;
    if(qtype == A || qtype == NS || qtype == CNAME){ // 缓存中只存放这几种类型的资源
        it = getItem(name,qtype);
        if(it != NULL){ // 直接就查到了
            Item2Msg(replyMsg,it,it->name);
#if DEBUG
            log_debug("-----------------");
            checkItem(it);
            log_debug("-----------------");
#endif
            free(it);
            setResp(replyMsg); // 设为响应报文
            setFlag(replyMsg,RA); // 设置flag
            n = encode(replyMsg,buff);
            sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr));
            log_info("get resource record in cache and respon to client !");
            destroyMsg(replyMsg);
            free(parg);
            return;
        } else if(qtype == A){ // 尝试能不能找到cname和cname对应的A
            it = getItem(name,CNAME);
            if(it != NULL){ // 找到了CNAME记录，还有机会找到A记录
                tit = getItem(it->strVal[0],A);
                if(tit != NULL){ // 找到了，完美
                    Item2Msg(replyMsg,it,it->name);
                    Item2Msg(replyMsg,tit,tit->name);
#if DEBUG
                    log_debug("-----------------");
                    checkItem(it);
                    log_debug("-----------------");
                    checkItem(tit);
                    log_debug("-----------------");

#endif
                    free(it);
                    free(tit);
                    setResp(replyMsg); // 设置为响应报文
                    setFlag(replyMsg,RA); // 设置flag
                    n = encode(replyMsg,buff);
#if DEBUG
                    tmpSend(buff,n);
                    showMsg(replyMsg);
                    showMem(buff,n);
#endif
                    sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr));
                    log_info("get CNAME and A resource record in cache and respon to client !");
                    destroyMsg(replyMsg);
                    free(parg);
                    return;
                }
                free(tit); // 别忘了释放空间
            }
        }
    }
    // 以上是从缓存查找资源记录的过程
    // 接下来尝试在文件中查找A记录

    // 还没有从文件中查找的功能捏，暂时略过

    //接下来开始中继功能
    //创建socket发送请求
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t servaddr_len;

    sockfd = Socket(AF_INET,SOCK_DGRAM,0); // 创建socket

    // 设置接收超时时间 1s
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof (tv));

    // 设置服务器ip+端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(53);
    inet_pton(AF_INET, "192.168.43.1", &servaddr.sin_addr);

    if(sendto(sockfd,parg->buff,parg->n,0,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){// 转发报文到DNS服务器
        log_warn("something went wrong when try to sent message to local DNS server ! err:%s",strerror(errno));
        setResp(replyMsg);
        setRCODE(replyMsg,SERVER_FAILURE);
        n = encode(replyMsg,buff);
        sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr)); // 回复服务器错误
        destroyMsg(replyMsg);
        free(parg);
        return;
    }

    //接收服务器信息
    n = recvfrom(sockfd,buff,1024,0,NULL,0);
    if(n == -1){
        log_warn("something went wrong when try to recvfrom message to local DNS server ! err:%s",strerror(errno));
        setResp(replyMsg);
        setRCODE(replyMsg,SERVER_FAILURE);
        n = encode(replyMsg,buff);
        sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr)); // 回复服务器错误
        destroyMsg(replyMsg);
        free(parg);
        return;
    }
    close(sockfd);// 关闭该临时socket
    sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr)); // 直接把从服务器得到的报文返回回去
    log_info("reply with the message from local DNS server");
    // 接下来要缓存服务器发回来的资源记录了
    RR *rr;
    item Ait,Nit,Cit; // 分别存放A类型记录 NS类型记录 CNAME类型记录
    bzero(&Ait,sizeof(item));
    bzero(&Nit,sizeof(item));
    bzero(&Cit,sizeof(item));

    destroyMsg(replyMsg);
    free(parg);
    replyMsg = decode(buff); // 解析服务器回复的报文
    for(int i=0;i<replyMsg->RR_count[ANSWER];i++){
        rr = replyMsg->resourse_record[ANSWER][i];
        if(rr->type == A)
            addRR2Item(&Ait,rr);
        else if(rr->type == NS)
            addRR2Item(&Nit,rr);
        else if(rr->type == CNAME)
            addRR2Item(&Cit,rr);
    }
    destroyMsg(replyMsg);
    putItem(Ait.name,Ait.type,&Ait);
    putItem(Nit.name,Nit.type,&Nit);
    putItem(Cit.name,Cit.type,&Cit);
}

void tmpSend(void *buff,size_t n){
    log_debug("started tmpSend");
    //创建socket发送请求
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t servaddr_len;

    sockfd = Socket(AF_INET,SOCK_DGRAM,0); // 创建socket
    // 设置服务器ip+端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(53);
    if(!(inet_pton(AF_INET, "192.168.43.1", &servaddr.sin_addr))){
        fprintf(stderr,"error occur when parsing dns server ip:%s\n","192.168.43.1");
        exit(-1);
    }
    // 向服务器发送信息
    sendto(sockfd,buff,n,0,(struct sockaddr*)&servaddr,sizeof(servaddr));

    log_debug("send to server");
    close(sockfd);
}

int main() {
    Pool tp = CreateThreadPool(10);
    log_info("Created a threadpool.Size of threadpool:%d",10);
    struct sockaddr_in cliAddr,global_addr;
    socklen_t cliLen;

    char str[101];
    clog_set_level(CLOG_LEVEL_DEBUG);
    // 设置好全局socket
    globalSocket = Socket(AF_INET,SOCK_DGRAM,0);
    bzero(&global_addr, sizeof(global_addr));
    global_addr.sin_family = AF_INET;
    global_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    global_addr.sin_port = htons(DNS_PORT);
    Bind(globalSocket,(struct sockaddr*)&global_addr, sizeof(global_addr));
    log_info("Set global socket.listen addr:%s:%d",inet_ntop(AF_INET,&global_addr.sin_addr,str,sizeof(str)),ntohs(global_addr.sin_port));

    globalCache = CreateCache(1024,NULL,NULL);
    log_info("initiate global cache.size:1024");

    int n;
    char buff[1024];
    arg *a;
    log_info("Accepting connections ...\n");
    for(;;){
        cliLen = sizeof(cliAddr);
        n = recvfrom(globalSocket,buff,1024,0,(struct sockaddr*)&cliAddr,&cliLen);
        if(n == -1)
            perr_exit("receive from error");

        log_info("received from %x:%s at PORT %d",cliAddr.sin_addr.s_addr,
               inet_ntop(AF_INET, &cliAddr.sin_addr, str, sizeof(str)),
               ntohs(cliAddr.sin_port));

#if DEBUG
        showMem(buff,n);
#endif
        a = malloc(sizeof(arg));
        a->cliAddr = cliAddr;
        memcpy(a->buff,buff,n);
        a->n = n;
        AddTask(tp,handler,a);
    }
    ClosePool(tp);
    return 0;
}
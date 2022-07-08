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
#include "clog.h"

#include "file_reader.h"

#include "arg.h"

#define STR_LEN 128
#define MAX_NUM 10


typedef struct {
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


int globalSocket; // 和客户端通信的socket
Cache *globalCache;
struct file_reader *localReader;
struct sockaddr_in servaddr; // 设置Local DNS server的地址

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
            log_info("domain blocked !! name:%s respon with a NAME_ERR error",name);
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
    char key[501];
    calcKey(name,type,key);
    return CacheGet(globalCache,key);
}

// 将一个条目放入缓存中
void putItem(char *name,int type,item *it){
    char key[501];
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
void showRequest(message *msg,enum clog_level level){
    static char *typename[50] = {NULL};
    typename[A] = "A";
    typename[CNAME] = "CNAME";
    typename[NS] = "NS";
    typename[AAAA] = "AAAA";
    typename[PTR] = "PTR";
    char *p = typename[msg->ques[0]->q_type];
    clog_log(level,"request for:%s type:%s",msg->ques[0]->q_name,p==NULL?"???":p);
}

void handler(void *ar) {
    char buff[1024];
    int n;
    arg *parg = ar;
    message *replyMsg = decode(parg->buff); // 要回复的msg
    releaseAdditionalRR(replyMsg); //把额外的段删去，不知为啥dig的请求中有时会带有additional段
    setResp(replyMsg); // 设置为响应报文
    setFlag(replyMsg,RA); // 设置flag
    showRequest(replyMsg,CLOG_LEVEL_INFO);

    char name[STR_LEN];
    strncpy(name,replyMsg->ques[0]->q_name,STR_LEN); // name放着将查询的域名字符串
    uint16_t qtype = replyMsg->ques[0]->q_type; //qtype中存放着将查询的资源类型

    // 下面尝试从缓存中读取资源记录
    item *it,*tit;
    if(qtype == A || qtype == NS || qtype == CNAME){ // 缓存中只存放这几种类型的资源
        it = getItem(name,qtype);
        if(it != NULL){ // 直接就查到了
            Item2Msg(replyMsg,it,it->name);
            free(it);
            n = encode(replyMsg,buff);
            sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr));
            log_info("get resource record in cache and respon to client !\n");
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
                    free(it);
                    free(tit);
                    n = encode(replyMsg,buff);
                    sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr));
                    log_info("get CNAME and A resource record in cache and respon to client !\n");
                    destroyMsg(replyMsg);
                    free(parg);
                    return;
                }
                free(tit); // 别忘了释放空间
            }
        }
    }
    // --------------------------------------------------------------
    // 以上是从缓存查找资源记录的过程
    // 接下来尝试在文件中查找A记录
    char ip[17];
    struct sockaddr_in tmpAddr;
    RR r;
    item tmp;
    bzero(&tmp, sizeof(item));
    bzero(&r, sizeof(RR));
    if(qtype == A){
        if(file_reader_get_a_record(localReader,name,ip) != NULL){ // 还真找到了
            inet_pton(AF_INET, ip, &tmpAddr.sin_addr);
            r.type = A;
            r.class = IN;
            r.TTL = 600000; // 随便设置一个长的TTL，因为本地的记录理论上来说是固定的
            setRRName(&r,name);
            setRRData(&r,&tmpAddr.sin_addr.s_addr, sizeof(uint32_t));
            // 以上构造了一个RR
            addRR2Item(&tmp,&r); // 将RR放入一个缓存项中
            putItem(tmp.name,A,&tmp); // 将缓存项放入缓存中
            Item2Msg(replyMsg,&tmp,tmp.name); // 将该缓存项中的内容放入message中
            n = encode(replyMsg,buff);
            sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr));
            log_info("get A resource record from local file and respon to client !");
            log_info("name:%s ip:%s\n",name,ip);
            destroyMsg(replyMsg);
            free(parg);
            return ;
        }
    }
    // ---------------------------------------------------------------------------
    //接下来开始中继功能
    //创建socket发送请求
    int sockfd;

    sockfd = Socket(AF_INET,SOCK_DGRAM,0); // 创建socket

    // 设置接收超时时间 4s
    struct timeval tv;
    tv.tv_sec = 4;
    tv.tv_usec = 0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof (tv));


    if(sendto(sockfd,parg->buff,parg->n,0,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){// 转发报文到DNS服务器
        showRequest(replyMsg,CLOG_LEVEL_INFO);
        log_warn("something went wrong when try to sent message to local DNS server ! err:%s\n",strerror(errno));
        setResp(replyMsg);
        setRCODE(replyMsg,SERVER_FAILURE);
        n = encode(replyMsg,buff);
        sendto(globalSocket,buff,n,0,(struct sockaddr*)(&parg->cliAddr), sizeof(parg->cliAddr)); // 回复服务器错误
        destroyMsg(replyMsg);
        free(parg);
        return;
    }
    log_info("send message to Local DNS server and wait for response");
    //接收服务器信息 不需要知道服务器地址了，后面两参数填 NULL 0
    n = recvfrom(sockfd,buff,1024,0,NULL,0);
    if(n == -1){
        log_warn("something went wrong when try to recvfrom message from local DNS server ! err:%s\n",strerror(errno));
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
    log_info("reply with the message from local DNS server\n");
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

void showHelp(){
    printf("usage:\n"
           "    server localfile=filename dns=local_dns_server_ip [options]\n"
           "\n"
           "    local_dns_server_ip is ip of your Local DNS server.When our program work as a relayer,it will send\n"
           "    message to the local_dns_server_ip.\n"
           "\n"
           "    filename is the file that stores a series of domain names and their ipv4 addresses.Each line is in\n"
           "    the form of: ip domain name.\n"
           "    e.g.  dnsrelay.txt\n"
           "    ...\n"
           "    74.125.207.113 i2.ytimg.com\n"
           "    74.125.207.113 i3.ytimg.com\n"
           "    ...\n"
           "\n"
           "options:\n"
           "    debug=level\n"
           "        level can be one of trace,debug,info,warn,error and fatal.Default level is info.\n"
           "\n"
           "    poolsize=num_of_thread_pool\n"
           "        Set the size of thread pool. Default size is 10.Size should not be more than 150.\n"
           "\n"
           "\n"
           "    cachesize=size\n"
           "        Set the size of cache, Default is 1024.\n"
           "\n"
           "    port=portnumber\n"
           "        Set the listening port number to portnumber. Default is 53.\n"
           "\n"
           "    --help\n"
           "        Show this content.");
}

int main(int argc,char **argv) {
    Arg *argument = NewArg(argc,argv);
    if(matchArg(argument,"--help","true")){
        showHelp();
        return 0;
    }

    int poolsize = getInt(argument,"poolsize");
    poolsize = poolsize == 0 ? 10 : poolsize;
    poolsize = poolsize > 150 ? 150 : poolsize;

    Pool tp = CreateThreadPool(poolsize);
    log_info("Created a threadpool.Size of threadpool:%d",poolsize);
    struct sockaddr_in cliAddr,global_addr;
    socklen_t cliLen;

    char str[181];

    enum clog_level debugLev = CLOG_LEVEL_INFO;
    if(matchArg(argument,"debug","trace")){
        log_info("set debug level:trace");
        debugLev = CLOG_LEVEL_TRACE;
    }else if(matchArg(argument,"debug","debug")){
        log_info("set debug level:debug");
        debugLev = CLOG_LEVEL_DEBUG;
    }else if(matchArg(argument,"debug","info")){
        log_info("set debug level:info");
        debugLev = CLOG_LEVEL_INFO;
    }else if(matchArg(argument,"debug","warn")){
        log_info("set debug level:warn");
        debugLev = CLOG_LEVEL_WARN;
    }else if(matchArg(argument,"debug","error")){
        log_info("set debug level:error");
        debugLev = CLOG_LEVEL_ERROR;
    }else if(matchArg(argument,"debug","fatal")){
        log_info("set debug level:debug");
        debugLev = CLOG_LEVEL_FATAL;
    }else
        log_info("set debug leve:info (default level)");
    clog_set_level(debugLev);
    // 设置好全局socket
    globalSocket = Socket(AF_INET,SOCK_DGRAM,0);
    bzero(&global_addr, sizeof(global_addr));
    global_addr.sin_family = AF_INET;
    global_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    uint16_t PORT;
    PORT = getInt(argument,"port");
    PORT = PORT == 0 ? 53 : PORT;
    global_addr.sin_port = htons(PORT);
    Bind(globalSocket,(struct sockaddr*)&global_addr, sizeof(global_addr));
    log_info("Set global socket.listen addr:%s:%d",inet_ntop(AF_INET,&global_addr.sin_addr,str,sizeof(str)),ntohs(global_addr.sin_port));

    int cachesize = getInt(argument,"cachesize");
    cachesize = cachesize <= 0 ? 1024 : cachesize;
    globalCache = CreateCache(cachesize,NULL,NULL);
    log_info("initiate global cache.size:%d\n",cachesize);

    char *p = getStr(argument,"localfile");
    if(p == NULL){
        log_fatal("localfile needed !!! use --help for help");
        return 0;
    }
    localReader = file_reader_alloc(p);
    log_info("initate local file reader:%s",p);

    // 设置服务器ip+端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(53);
    p = getStr(argument,"dns");
    if(p == NULL){
        log_fatal("dns needed !!! use --help for help");
        return 0;
    }
    if(!inet_pton(AF_INET, p, &servaddr.sin_addr)){
        log_fatal("error:can not parse ip of Local DNS server");
        return 0;
    }
    log_info("set DNS ip:%s",p);

    DestroyArg(argument); // 清理存储的命令行参数

    int n;
    char buff[1024];
    arg *a;
    log_info("Accepting connections ...\n");
    for(;;){
        cliLen = sizeof(cliAddr);
        n = recvfrom(globalSocket,buff,1024,0,(struct sockaddr*)&cliAddr,&cliLen);
        if(n == -1)
            perr_exit("receive from error");

        log_info("received from %s at PORT %d",
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
    DestroyCache(globalCache);
    close(globalSocket);
    file_reader_free(localReader);
    return 0;
}
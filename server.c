////
//// Created by 谢卫凯 on 2022/3/31.
////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "wrap.h"

#include "thread_pool.h"
#include "message.h"
#include "cache.h"
#include "log.h"

#define STR_LEN 64

#define DNS_PORT 8080

typedef struct {
    message *msg;
    struct sockaddr_in cliAddr;
}arg;

// 以下这个结构存入缓存
// key为name:type
// 如www.baidu.com:A这个key中存放着的item的type为A ipv4顾名思义，DDL是过期时间，strVal当type为CNAME NS之类的时候才用
typedef struct {
    uint8_t type;
    uint32_t ipv4;
    char strVal[STR_LEN + 1];
    time_t ddl; // 过期时间
}item;

// 依据name type 返回key key存入res中，成功则返回true,失败返回false
// 如调用calcKey("www.baidu.com",CNAME,buf)
// 调用后buf中存的字符串为 "www.baidu.com:CNAME"
// 调用方要确保res 足够长
int calcKey(char *name,int type,char *res){
    static char *typename[30] = {NULL};
    typename[A] = "A";
    typename[AAAA] = "AAAA";
    typename[CNAME] = "CNAME";
    typename[NS] = "NS";
    if(type <= 0 || type >= 30 || typename[type] == NULL) // 无效的type
        return false;
    res[0] = 0;
    strcat(res,name);
    strcat(res,":");
    strcat(res,typename[type]);
    return true;
}

int globalSocket;
Cache *globalCache;

void handler(void *ar){
    char buff[1024],key[50];
    int n;
    arg *parg = ar;
    message *replyMsg = parg->msg; // 要回复的msg
    RR rr;

    char *name = parg->msg->ques[0]->q_name; // name放着将查询的域名字符串
    uint16_t qtype = parg->msg->ques[0]->q_type; //qtype中存放着将查询的资源类型
    calcKey(name,qtype,key);
    item *it = CacheGet(globalCache,key);
    if(it == NULL && qtype == CNAME){ // 有可能查到CNAME 然后CNAME里面有
        calcKey(name,CNAME,key);
        it = CacheGet(globalCache,key);
        if(it != NULL){
            
        }
    }

    if(it != NULL){// 在缓存中查到了该资源
        setResp(replyMsg);
        setFlag(replyMsg,RA);

        setRRName(&rr,name);
        if(qtype == CNAME || qtype == NS)
            setRRNameData(&rr,it->strVal);
        else
            setRRData(&rr,&it->ipv4,sizeof(it->ipv4)); // 暂时只支持A类型，就存一个A类型资源

        rr.type = qtype;
        rr.class = IN;
        rr.TTL = it->ddl - time(NULL);
        addRR(replyMsg,&rr,ANSWER);
        n = encode(replyMsg,buff);
        sendto(globalSocket,buff,n,0,(struct sockaddr*)&parg->cliAddr, sizeof(parg->cliAddr)); // 回复用户
        log_info("get resource record from cache and reply !!");
    }
}

int main(){
    char res[100];
    calcKey("www.baidu.com",CNAME,res);
    log_info(res);
    calcKey("google.com",A,res);
    log_info(res);
//    Pool tp = CreateThreadPool(10);
//    log_info("Created a threadpool.Size of threadpool:%d",10);
//    struct sockaddr_in cliAddr,global_addr;
//    socklen_t cliLen;
//
//    char str[101];
//
//    // 设置好全局socket
//    globalSocket = Socket(AF_INET,SOCK_DGRAM,0);
//    bzero(&global_addr, sizeof(global_addr));
//    global_addr.sin_family = AF_INET;
//    global_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//    global_addr.sin_port = htons(DNS_PORT);
//    Bind(globalSocket,(struct sockaddr*)&global_addr, sizeof(global_addr));
//    log_info("Set global socket.listen addr:%s:%d",inet_ntop(AF_INET,&global_addr.sin_addr,str,sizeof(str)),ntohs(global_addr.sin_port));
//
//    globalCache = CreateCache(1024,NULL,NULL);
//    log_info("initiate global cache.size:1024");
//
//    int n;
//    char buff[1024];
//    message *msg;
//    arg *a;
//    log_info("Accepting connections ...\n");
//    for(;;){
//        cliLen = sizeof(cliLen);
//        n = recvfrom(globalSocket,buff,1024,0,(struct sockaddr*)&cliAddr,&cliLen);
//        if(n == -1)
//            perr_exit("receive from error");
//        log_info("received from %s at PORT %d\n",
//               inet_ntop(AF_INET, &cliAddr.sin_addr, str, sizeof(str)),
//               ntohs(cliAddr.sin_port));
//        msg = decode(buff);
//        a = malloc(sizeof(arg));
//        a->cliAddr = cliAddr;
//        a->msg = msg;
//        AddTask(tp,handler,a);
//    }
//    ClosePool(tp);
    return 0;
}
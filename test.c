//
// Created by 谢卫凯 on 2022/4/25.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>

#include "thread_pool.h"
#include "cache.h"
#include "wrap.h"
#include "message.h"

char *name[] = {"xwk","wql","yr","hr","yyj","ysfj"};
int value[] = {1,2,3,4,5,6};
int count[6] = {0};

void sayHello(void *n){
    printf("Hello %s\n",(char *)n);
    for(int i=0;i<6;i++)
        if(strncmp(name[i],(char*)n, 4) == 0)
            count[i] ++;
}

void testPool(){
    Pool p = CreateThreadPool(100);
    for(int i=0;i<1000;i++){
        for(int j=0;j<6;j++)
            AddTask(p,sayHello,name[j]);
    }

    sleep(5);
    ClosePool(p);
    for(int i=0;i<6;i++)
        printf("count[%d] = %d\n",i,count[i]);

}
/*------------------------------above is testPool-------------------------------*/

typedef struct {
    char *key;
    int *v;
}Val;

void *copyVal(void *data){
    Val *tmp = data;
    Val *res;
    res = malloc(sizeof(Val));
    if(res == NULL){
        fprintf(stderr,"malloc error !!");
        exit(-1);
    }
    res->key = malloc(strlen(tmp->key)+1);
    if(res->key == NULL){
        fprintf(stderr,"malloc error !!");
        exit(-1);
    }
    strcpy(res->key,tmp->key);
    res->v = malloc(sizeof(int));
    if(res->v == NULL){
        fprintf(stderr,"malloc error !!");
        exit(-1);
    }
    *(res->v) = *(tmp->v);
    return res;
}

void deleteVal(void *data){
    Val *tmp = data;
    free(tmp->key);
    free(tmp->v);
    free(tmp);
}

//pthread_mutex_t ttt = PTHREAD_MUTEX_INITIALIZER;

void putInCache(void *arg){
    printf("get into putInCache\n");
    Cache *c = arg;
    int pos;
    Val *v;
    for(int i=0;i<50000;i++){
        pos = rand()%6;
        v = malloc(sizeof(Val));
        v->key = malloc(10);
        strcpy(v->key,name[pos]);
        v->v = malloc(sizeof(int));
        *(v->v) = value[pos];
//        pthread_mutex_lock(&ttt);
        CachePut(c,name[pos],v, sizeof(Val),rand()%10+1);
//        pthread_mutex_unlock(&ttt);
        deleteVal(v);
    }
    printf("get out of putInCache\n");
}

void getInCache(void *arg){
    printf("get into getInCache\n");
    Cache *c = arg;
    Val *v;
    int pos;
    for(int i=0;i<50000;i++){
        pos = rand() % 6;
//        pthread_mutex_lock(&ttt);
        v = CacheGet(c,name[pos]);
//        pthread_mutex_unlock(&ttt);
        if(v != NULL) {
            if (*(v->v) != value[pos]) {
                fprintf(stderr, "error:can not get the expected value!!!!!\n");
            }else printf("pass! key:%s value:%d\n",v->key,*(v->v));
            deleteVal(v);
        }
    }
    printf("get out of getInCache\n");
}

void testCache1(){
    Pool p = CreateThreadPool(2);
    Cache *c = CreateCache(100,copyVal,deleteVal);
    AddTask(p,putInCache,c);
    AddTask(p,getInCache,c);
    sleep(20);
    ClosePool(p);
    DestroyCache(c);
}


void putInCache2(void *arg){
    printf("get into putInCache2\n");
    Cache *c = arg;
    int pos;

    for(int i=0;i<100000;i++){
        pos = rand()%6;
        CachePut(c,name[pos],&value[pos], sizeof(int),rand()%10+1);
    }
    printf("get out of putInCache2\n");
}

void getInCache2(void *arg){
    printf("get into getInCache2\n");
    Cache *c = arg;
    int *v;
    int pos;
    for(int i=0;i<100000;i++){
        pos = rand() % 6;
        v = CacheGet(c,name[pos]);
        if(v != NULL) {
            if (*v != value[pos]) {
                fprintf(stderr, "error:can not get the expected value!!!!!\n");
            }
            free(v);
        }
    }
    printf("get out of getInCache2\n");
}

void testCache2(){
    Pool p = CreateThreadPool(2);
    Cache *c = CreateCache(100,NULL,NULL);
    AddTask(p,putInCache2,c);
    AddTask(p,getInCache2,c);
    sleep(5);
    ClosePool(p);
    DestroyCache(c);
}

// -------------------------above is test Cache---------------------------------------//


void producer(void *arg){
    printf("get into producer\n");
    safequeue q = arg;
    int *p;
    for(int i=0;i <= 10000;i++){
        p = malloc(sizeof(int));
        *p = i;
        SafeEnque(q,p);
    }
    sleep(3);
    SafeClose(q);
    printf("get out of producer\n");
}

void consumer(void *arg){
    printf("get into consumer\n");
    safequeue q = arg;
    int *p;
    while ((p = SafeDeque(q)) != NULL){
        printf("receice:%d\n",*p);
        free(p);
    }
    printf("get out of consumer\n");
}

void testQueue(){
    Pool p = CreateThreadPool(15);
    safequeue q = NewSafeQueue();
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,consumer,q);
    AddTask(p,producer,q);
    sleep(1);
    printf("after sleep\n");
    ClosePool(p);
    SafeDestroy(q);
    printf("close pool\n");
}

#include "arg.h"

// 使用UDP将buff前size个字节发送到dst
// 例: sentTo("lala",4,"192.168.43.1:80");
void sentTo(void *buff,size_t size,char *dst){
    char dstTmp[100];
    int sock = Socket(AF_INET,SOCK_DGRAM,0);

    struct sockaddr_in dstAddr;
    bzero(&dstAddr, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;

    char *s;
    strncpy(dstTmp,dst,100);
    s = strtok(dstTmp,":");
    dstAddr.sin_addr.s_addr = inet_addr(s);
    s = strtok(NULL,":");
    dstAddr.sin_port = htons((uint16_t)atoi(s));

    sendto(sock,buff,size,0,(struct sockaddr*)&dstAddr,sizeof(dstAddr));

    close(sock);

}

int main(int argc,char **argv){
//    srand(time(NULL));
//    testCache1();

//    Arg *arg = NewArg(argc,argv);
//
//    printf("check --help:%d\n", matchArg(arg, "--help", "true"));
//    printf("num:%d\n",getInt(arg,"num"));
//    printf("str:%s\n",getStr(arg,"str"));
//
//    DestroyArg(arg);


//    message *msg = newMsg();
//    setResp(msg);
//    msg->ID = 0x1234;
//    setOpcode(msg,QUERY);
//    setFlag(msg,RD);
//    setFlag(msg,RA);
//    setRCODE(msg,NAME_ERR);

//    question q;
//    setQNAME(&q,"www.baidu.com");
//    q.q_type = A;
//    q.q_class = IN;
//    addQuestion(msg,&q);
//
//    size_t n;
//    char buff[1024];
//    n = encode(msg,buff);
//    destroyMsg(msg);
//
//    sentTo(buff,n,"192.168.43.1:53");

    testQueue();
    return 0;
}
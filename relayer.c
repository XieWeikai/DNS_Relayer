//
// Created by 谢卫凯 on 2022/5/21.
//



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

#define COLOR_NONE "\033[0m"
#define RED "\033[1;31m"

#define DNS_PORT 8080
#define DNS_IP "192.168.43.1"

typedef struct {
    message *msg;
    struct sockaddr_in cliAddr;
}arg;

int globalSocket;

void handler(void *ar){
    printf(RED"get into handler !!!\n"COLOR_NONE);
    char buff[1024];
    int n;
    arg *pa = ar;
    n = encode(pa->msg,buff);

    //创建socket发送请求
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t servaddr_len;

    sockfd = Socket(AF_INET,SOCK_DGRAM,0); // 创建socket
    // 设置服务器ip+端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(53);
    if(!(inet_pton(AF_INET, DNS_IP, &servaddr.sin_addr))){
        fprintf(stderr,RED"error occur when parsing dns server ip:%s\n"COLOR_NONE,DNS_IP);
        return ;
    }
    // 向服务器发送信息
    if(sendto(sockfd,buff,n,0,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
        perr_exit("sendto error");
    printf(RED"sendto server and waiting for respone ......\n"COLOR_NONE);
    // 设置接收超时时间 1s
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof (tv));

    //接收服务器信息
    n = recvfrom(sockfd,buff,1024,0,NULL,0);
    if(n == -1) {
        fprintf(stderr, RED"failto get message from DNS server !\n"COLOR_NONE);
        return;
    }
    // 回复用户
    sendto(globalSocket,buff,n,0,(struct sockaddr*)&pa->cliAddr, sizeof(pa->cliAddr));
    printf(RED"sent to user\n"COLOR_NONE);

    // 释放msg空间
    destroyMsg(pa->msg);
    free(ar); // 释放参数空间
    Close(sockfd); // 关闭该socket
}

int main(){
    Pool tp = CreateThreadPool(10);
    struct sockaddr_in cliAddr,global_addr;
    socklen_t cliLen;

    // 设置好全局socket
    globalSocket = Socket(AF_INET,SOCK_DGRAM,0);
    bzero(&global_addr, sizeof(global_addr));
    global_addr.sin_family = AF_INET;
    global_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    global_addr.sin_port = htons(DNS_PORT);
    Bind(globalSocket,(struct sockaddr*)&global_addr, sizeof(global_addr));

    int n;
    char buff[1024];
    char str[101];
    message *msg;
    arg *a;
    printf("Accepting connections ...\n");
    for(;;){
        cliLen = sizeof(cliLen);
        n = recvfrom(globalSocket,buff,1024,0,(struct sockaddr*)&cliAddr,&cliLen);
        if(n == -1)
            perr_exit("receive from error");
        printf("received from %s at PORT %d\n",
               inet_ntop(AF_INET, &cliAddr.sin_addr, str, sizeof(str)),
               ntohs(cliAddr.sin_port));
        msg = decode(buff);
        a = malloc(sizeof(arg));
        a->cliAddr = cliAddr;
        a->msg = msg;
        AddTask(tp,handler,a);
    }
    ClosePool(tp);
    return 0;
}

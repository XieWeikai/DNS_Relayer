////
//// Created by 谢卫凯 on 2022/3/31.
////

#define MY 0

#if !MY
#include <stdio.h>
#include <sys/socket.h>
#include <dns_util.h>
#include <unistd.h>

#include "message.h"

void *decodeName(void *buff,void *origin,char *name);

int main(){
    char buff[1024];
    message *msg = newMsg();
    question q;
    msg->ID = 0x8d17;
    setQuery(msg);
    setFlag(msg,RD);
    setFlag(msg,AD);
    q.q_type = A;
    q.q_class = IN;
    setQNAME(&q,"www.baidu.com");
    addQuestion(msg,&q);
    ssize_t n = encode(msg,buff);
    showMem(buff,n);
    showMsg(msg);
    destroyMsg(msg);
    // 构造报文结束


    int fd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in servaddr;

    message *mmm;
    servaddr.sin_port = htons(53);
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.43.1", &servaddr.sin_addr);
    sendto(fd,buff,n,0,(struct sockaddr *)&servaddr,sizeof (servaddr));
    printf("after send to\n");
    n = recvfrom(fd,buff,1024,0,NULL,0);
    showMem(buff,n);
    mmm = decode(buff);
    showMsg(mmm);
    n = encode(mmm,buff);
    showMem(buff,n);
    msg = decode(buff);
    showMsg(msg);


//    int fd1;
//    char str[50];
//    struct sockaddr_in serAddr,remoteAddr, addr;
//    socklen_t len = sizeof (addr);
//    remoteAddr.sin_port = htons(53);
//    inet_pton(AF_INET, "192.168.43.1", &remoteAddr.sin_addr);
//    remoteAddr.sin_family = AF_INET;
//
//    serAddr.sin_port = htons(43);
//    serAddr.sin_addr.s_addr = htons(INADDR_ANY);
//    serAddr.sin_family = AF_INET;
//    fd1 = socket(AF_INET,SOCK_DGRAM,0);
//    bind(fd1,&serAddr,sizeof (serAddr));
//    int pid;
//    if((pid = fork()) == 0){
//        printf("child receive from\n");
//        n = recvfrom(fd1,buff,1024,0,&addr,&len);
//        if(n == -1)
//            perror("receive error");
//        printf("child receive from %s:%d-----\n",
//               inet_ntop(AF_INET, &addr.sin_addr, str, sizeof(str)),
//               ntohs(addr.sin_port));
//        showMem(buff,n);
//    }else{
//        sleep(5);
//        printf("parent send to\n");
//        showMem(buff,n);
//        sendto(fd1,buff,n,0,&serAddr,sizeof (remoteAddr));
//        if(n == -1)
//            perror("send to error");
//    }
//
//    sleep(10);
    return 0;
}

#endif

#if MY
//#include <stdio.h>
//#include <inttypes.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
//#include <wrap.h>
//#include <dns_sd.h>
//#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <dns_util.h>
#include <ctype.h>
#include <unistd.h>
#include "wrap.h"

#define MAX_SIZE 1024

#define PORT 8000

int main(){
    struct sockaddr_in serverAddr,cliAddr;
    socklen_t cliAddrLen = sizeof (cliAddrLen);
    char str[MAX_SIZE];

    int fd;
    fd = Socket(AF_INET,SOCK_DGRAM,0);

    bzero(&serverAddr,sizeof (serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    Bind(fd,(struct sockaddr*)&serverAddr,sizeof (serverAddr));

    printf("start to receive message!\n");
    char buff[MAX_SIZE];
    int n;
    while(1){
        n = recvfrom(fd,buff,MAX_SIZE,0,(struct sockaddr*)&cliAddr,&cliAddrLen);
        //sleep(1);
        if(n == -1){
            perr_exit("error");
            fprintf(stderr,"recvfrom error! %s\n",strerror(errno));
            continue;
        }
        printf("receive from %s:%d\n",
               inet_ntop(AF_INET, &cliAddr.sin_addr, str, sizeof(str)),
               ntohs(cliAddr.sin_port));
        for(int i=0;i<n;i++) {
            buff[i] = toupper(buff[i]);
            printf("%c",buff[i]);
        }
        printf("\n");
        if(buff[0] == 'B' && buff[1] == 'Y' && buff[2] == 'E'){
            sendto(fd,"bye\n",4,0,(struct sockaddr*)&cliAddr,sizeof (cliAddr));
            break;
        }
        n = sendto(fd,buff,n,0,(struct sockaddr*)&cliAddr,sizeof (cliAddr));

        if(n == -1)
            fprintf(stderr,"sendtoErr:%s\n",strerror(errno));
    }
    close(fd);
    printf("close(fd)\n");
    return 0;
}
#endif

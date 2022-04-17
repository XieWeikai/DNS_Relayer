//
// Created by 谢卫凯 on 2022/4/1.
//

/* client.c */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message.h"
#include "wrap.h"

int main(int argc, char *argv[])
{
    char dnsIp[51];
    char hostName[101];
    char buff[1024]; // 装报文用的内存
    ssize_t n; //报文大小

    printf("dns server ip:");
    scanf("%s",dnsIp);
    printf("hostname:");
    scanf("%s",hostName);

    message *msg = newMsg();
    // 设置请求头
    setQuery(msg);
    msg->ID = 0x1234; // 两字节ID，随便给的
    setOpcode(msg,QUERY);
    setFlag(msg,RD);

    // 设置question section
    question q;
    setQNAME(&q,hostName);
    q.q_type = A; q.q_class = IN;
    addQuestion(msg,&q);

    // 将msg编为报文
    n = encode(msg,buff);
    destroyMsg(msg); //释放空间

    //创建socket发送请求
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t servaddr_len;

    sockfd = Socket(AF_INET,SOCK_DGRAM,0); // 创建socket
    // 设置服务器ip+端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(53);
    if(!(inet_pton(AF_INET, dnsIp, &servaddr.sin_addr))){
        fprintf(stderr,"error occur when parsing dns server ip:%s\n",dnsIp);
        exit(-1);
    }
    // 向服务器发送信息
    if(sendto(sockfd,buff,n,0,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
        perr_exit("sendto error");
    printf("sendto server and waiting for respone ......\n");
    // 设置接收超时时间 5s
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof (tv));

    //接收服务器信息
    n = recvfrom(sockfd,buff,1024,0,NULL,0);
    if(n == -1)
        perr_exit("recvfrom error");

    msg = decode(buff); // 将报文转化为message结构
    printf("\n\nrecv message!!\n");
    showMsg(msg); //展示报文
    printf("\n\nmemory:\n");
    showMem(buff,n);  // 展示内存

    // 试一下编码能不能和发来的一样
    printf("\n\nthis is a test!!\n");
    n = encode(msg,buff);
    showMem(buff,n);
    return 0;
}
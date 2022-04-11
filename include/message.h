//
// Created by 谢卫凯 on 2022/4/1.
//

#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include <ctype.h>
#include <stdlib.h>

#define MAX_LEN 256
#define MAX_SIZE 32

//type value
#define A 1
#define NS 2 //an authoritative name server
#define CNAME 5 //the canonical name for an alias
#define AAAA 28

//class value
#define IN 1 //the Internet
#define CS 2 // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
#define CH 3 // the CHAOS class
#define HS 4 // Hesiod [Dyer 87]

//opcode
#define QUERY 0 //a standard query (QUERY)
#define IQUERY 1  //an inverse query (IQUERY)
#define STATUS 2  //a server status request (STATUS)

//Rcode
#define NO_ERR 0 // no error
#define FMT_ERR 1 //Format error - The name server wasunable to interpret the query.
#define SERVER_FAILURE 2  //Server failure - The name server was unable to process this query due to a problem with the name server.
#define NAME_ERR 3 //domain name referenced in the query does not exist.

// flag
#define AA 5 //Authoritative Answer
#define TC 6 //TrunCation
#define RD 7 // Recursion Desired
#define RA 8 //Recursion Available
#define AD 10 //1 为应答服务器已经验证了该查询相关的 DNSSEC 数字签名 (RFC1035中没有)抓包看到的，网上查到的
#define CD 11 //1 为服务器并未进行相关 DNSSEC 数字签名的验证       (RFC1035中没有)抓包看到的，网上查到的

// RR section
#define ANSWER 0
#define AUTHORITY 1
#define ADDITIONAL 2

// data type
#define STRING_TYPE 0
#define BINARY_TYPE 1

// question对应的结构
typedef struct ques{
    char q_name[MAX_LEN];
    uint16_t q_type;
    uint16_t q_class;
}question;

// resource record 对应的结构
typedef struct {
    char name[MAX_LEN];
    uint16_t type;
    uint16_t class;
    uint32_t TTL;
    uint16_t data_length;
    void *data;
    uint8_t data_type;
    char string_data[MAX_LEN];
}RR;

// message 结构
typedef struct msg{
    uint16_t ID;
    uint16_t flag;
    uint16_t q_count;
    uint16_t RR_count[3]; // RR_count[ANSWER]代表answer段数量 以此类推
//    uint16_t ans_count;
//    uint16_t auth_count;
//    uint16_t add_count;

    question *ques[MAX_SIZE];
    RR *resourse_record[3][MAX_SIZE]; // resourse_record[ANSWER]代表answer段，以此类推
//    RR *ans[MAX_SIZE];
//    RR *auth[MAX_SIZE];
//    RR *add[MAX_SIZE];
}message;

//创建新报文
message *newMsg();

//设置报文为请求报文
void setQuery(message *msg);

//设置报文为响应报文
void setResp(message *msg);

//设置操作码
void setOpcode(message *msg,uint16_t op);

//设置某个标志位
//如要设置AA位，则这样调用  setFlag(msg,AA)
//相应的宏已经预先写好 包括 AA TC RD RA
void setFlag(message *msg,uint16_t b);

//设置响应码
void setRCODE(message *msg,uint16_t rcode);

//设置QNAME
void setQNAME(question *q,char *name);

//为报文添加一个问题
//失败返回0
int addQuestion(message *msg,question *q);

//设置RR的name字段
void setRRName(RR *rr,char *s);

//设置RR的data字段为字符串
void setRRNameData(RR *rr,char *name);

//设置RR的data字段 二进制数据
void setRRData(RR *rr,void *data,size_t size);

// 添加一条资源记录
// 可以选择添加到 ANSWER AUTHORITY ADDITIONAL中的一段，由第三个参数标识
// 如可以这样用 addRR(msg,q,ANSWER)
int addRR(message *msg,RR *q,int type);

//释放一条message结构占用的空间
void destroyMsg(message *msg);

//将msg结构编码为可以实际发送的报文
//注意给够buff的长度
//否则会段错误，这里为了方便默认buff长度足够
//返回报文的长度 出错则返回-1
ssize_t encode(message *msg,void *buff);

//将报文buff变为结构体
message *decode(void *buff);

//调试用，打印一条报文
void showMsg(message *msg);

// 显示内存中的一段，调试用
void showMem(void *mem,size_t len);

#endif //DNS_MESSAGE_H

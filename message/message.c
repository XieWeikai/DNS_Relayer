//
// Created by 谢卫凯 on 2022/4/1.
//
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "message.h"

message *newMsg(){
    message *t = calloc(1,sizeof (message));
    if(t == NULL)
        return NULL;
    return t;
}

// 设该message为query
void setQuery(message *msg){
    msg->flag &= ~1;
}

// 设该message为响应
void setResp(message *msg){
    msg->flag |= 1;
}

//设置操作码
void setOpcode(message *msg,uint16_t op){
    msg->flag &= (0xf << 1);
    msg->flag |= (op << 1);
}

//设置某个标志位
//如要设置AA位，则这样调用  setFlag(msg,AA)
//相应的宏已经预先写好 包括 AA TC RD RA
void setFlag(message *msg,uint16_t b){
    msg->flag |= (1 << b);
}

//设置响应码
void setRCODE(message *msg,uint16_t rcode){
    msg->flag |= (rcode << 12);
}

//为报文添加一个问题
int addQuestion(message *msg,question *q){
    if(msg->q_count >= MAX_SIZE)
        return 0; // can not set more than MAX_SIZE questions

    question *tmp;
    tmp = malloc(sizeof (*tmp));
    if(tmp == NULL)
        return 0; // fail to allocate memory
    memcpy(tmp,q,sizeof (*tmp));
    msg->ques[msg->q_count++] = tmp;
    return 1;
}

// 添加一条资源记录
// 可以选择添加到 ANSWER AUTHORITY ADDITIONAL中的一段，由第三个参数标识
// 如可以这样用 addRR(msg,q,ANSWER)
int addRR(message *msg,RR *q,int type){
    if(msg->RR_count[type] >= MAX_SIZE)
        return 0; // there is no more room for new RR
    RR *tmp = malloc(sizeof (*tmp));
    if(tmp == NULL)
        return -1;
    memcpy(tmp,q,sizeof (*tmp));
    msg->resourse_record[type][msg->RR_count[type]++] = tmp;
    return 1;
}

//释放一条message结构占用的空间
void destroyMsg(message *msg){
    for(int i=0;i < msg->q_count;i++)
        free(msg->ques[i]);
    for(int i = 0;i < 3;i++){
        for(int j = 0;j < msg->RR_count[i];j++) {
            free(msg->resourse_record[i][j]->data);
            free(msg->resourse_record[i][j]);
        }
    }
    free(msg);
}

//翻转8个bits 纯属为了离奇的标志位编的这段
uint8_t reverse8(uint8_t b){
    b = ( b & 0x55 ) << 1 | ( b & 0xAA ) >> 1;
    b = ( b & 0x33 ) << 2 | ( b & 0xCC ) >> 2;
    b = ( b & 0x0F ) << 4 | ( b & 0xF0 ) >> 4;
    return b;
}

ssize_t encodeHeader(message *msg,void *buff){
    uint16_t *p = buff;
    *p ++ = htons(msg->ID);
//    *p ++ = htons(msg->flag);
// 这个标志位的顺序是真有够离谱，抓包看到的结果简直离奇
// 颠来倒去的 ....... 补救一下，写一段离奇的代码
    *(uint8_t *)p = reverse8(msg->flag & 0xff); p = (uint16_t *)((uint8_t *)p + 1);
    *(uint8_t *)p = reverse8((msg->flag >> 8) & 0xff); p = (uint16_t *)((uint8_t *)p + 1);
//离奇毕
    *p ++ = htons(msg->q_count);
    for(int i=0;i<3;i++)
        *p++ = htons(msg->RR_count[i]);
    return (ssize_t)((void *)p - buff);
}

// 将一个字符串转化为指定的格式
// 注：我的格式不使用压缩的方式
// 返回转化后的占用内存字节数
ssize_t encodeName(char *name,void *buff){
    char *num = buff,*p = buff+1;
    uint8_t cnt = 0;
    while(*name != 0){
        if(*name == '.'){
            *num = cnt;
            num = p++;
            cnt = 0;
            name++;
            continue;
        }
        *p++ = *name ++;
        cnt ++;
    }
    *num = cnt;
    *p++ = 0;
    return (ssize_t)((void *)p - buff);
}

// 将问题编进报文 返回占用字节数
ssize_t encodeQues(question *ques,void *buff){
    uint16_t *t = buff;
    ssize_t n = encodeName(ques->q_name,buff);
    t = (void *)t + n;
    *t++ = htons(ques->q_type);
    *t++ = htons(ques->q_class);
    return (ssize_t)((void *)t-buff);
}

// 将资源记录编进报文 返回占用字节数
ssize_t encodeRR(RR *rr,void *buff){
    uint16_t *t = buff;
    ssize_t n = encodeName(rr->name,buff);
    t = (void *)t + n;
    *t++ = htons(rr->type);
    *t++ = htons(rr->class);
    *((uint32_t *)t) = htonl(rr->TTL); t = (uint16_t *)((uint32_t *)t + 1);
    *t++ = htons(rr->data_length);
    memcpy(t,rr->data,rr->data_length);
    return (ssize_t)((void *)t - buff) + (ssize_t)(rr->data_length);
}

//为了简便name段不设置信息压缩
//将msg结构编码为可以实际发送的报文
//注意给够buff的长度
//否则会段错误，这里为了方便默认buff长度足够
//返回报文的长度 出错则返回-1
ssize_t encode(message *msg,void *buff){
    void *p = buff;
    ssize_t n;
    n = encodeHeader(msg,p);
    p = p + n;
    for(int i=0;i<msg->q_count;i++) {
        n = encodeQues(msg->ques[i], p);
        p = p + n;
    }
    for(int i =0;i<3;i++)
        for(int j = 0;j < msg->RR_count[i];j++){
            n = encodeRR(msg->resourse_record[i][j],p);
            p = p + n;
        }
    return (ssize_t)(p - buff);
}

// 解码以label形式存储的name
// origin 是整个报文的开头，因为某些name以Pointe为指针，要用到整个报文
// 返回解码后应该继续解析的位置
void *decodeName(void *buff,void *origin,char *name){
    uint8_t *p = (uint8_t *)(buff+1);
    uint8_t *num = buff;
    uint16_t tmp;
    while(*num != 0){
        if((uint8_t)(p-num) > *num){ //下一小节
            num = p++;
            *name++ = '.';
            continue;
        }
        if(*num >= 0xc0){ //以pointer结尾，递归的解析
            tmp = ((uint16_t)(*num++ & 0x3f)) & 0xff;
            tmp <<= 8;
            tmp |= (((uint16_t)(*num++)) & 0xff);
            decodeName(origin+tmp,origin,name);
            return num;
        }
        *name++ = *p++;
    }
    *(name-1) = 0;
    return (void *)p;
}

// 将报文变为message结构体
message *decode(void *buff){
    uint16_t *p = buff;
    uint8_t *tmp;
    message *msg = newMsg();
    msg->ID = ntohs(*p++);
    tmp = (uint8_t *)p;
    msg->flag = (uint16_t)(reverse8(*tmp++) & 0xff);
    msg->flag = msg->flag | (((int16_t)(reverse8(*tmp++)) & 0xff) << 8);
    p = (uint16_t *)tmp;
    msg->q_count = ntohs(*p++);
    for(int i=0;i<3;i++)
        msg->RR_count[i] = ntohs(*p++);
    for(int i=0;i < msg->q_count;i++){
        //printf("start a question\n");
        msg->ques[i] = malloc(sizeof (question));
        p = decodeName(p,buff,msg->ques[i]->q_name);
        msg->ques[i]->q_type = ntohs(*p++);
        msg->ques[i]->q_class = ntohs(*p++);
        //printf("name %s type:%d class:%d\n",msg->ques[i]->q_name,msg->ques[i]->q_type,msg->ques[i]->q_class);
    }
    RR *rr;
    for(int i=0;i < 3;i++){
        for(int j=0;j < msg->RR_count[i];j++){
            rr = malloc(sizeof (*rr));
            msg->resourse_record[i][j] = rr;
            p = decodeName(p,buff,rr->name);
            rr->type = ntohs(*p++);
            rr->class = ntohs(*p++);
            rr->TTL = ntohl(*(uint32_t *)p); p = (uint16_t *)((uint32_t *)p + 1);
            rr->data_length = ntohs(*p++);
            if(rr->type == CNAME || rr->type == NS) { //数据类型应该是字符串
                decodeName(p, buff, rr->string_data);
                rr->data_type = STRING_TYPE;
                rr->data = malloc(MAX_LEN+2);
                rr->data_length = encodeName(rr->string_data,rr->data);
            } else {
                rr->data_type = BINARY_TYPE;
                rr->data = malloc(rr->data_length); //data中存放原始的二进制信息
                memcpy(rr->data,p,rr->data_length);
            }
            p = (void *)p + rr->data_length;
        }
    }
    return msg;
}

//设置RR的name字段
void setRRName(RR *rr,char *s){
    strncpy(rr->name,s,MAX_LEN);
}

//设置RR的data字段为字符串
void setRRNameData(RR *rr,char *name){
    ssize_t n;
    rr->data = malloc(MAX_LEN);
    n = encodeName(name,rr->data);
    rr->data_length = n;
    rr->data_type = STRING_TYPE;
    strncpy(rr->string_data,name,MAX_LEN);
}

//设置RR的data字段
void setRRData(RR *rr,void *data,size_t size){
    rr->data_length = size;
    rr->data = malloc(size);
    rr->data_type = BINARY_TYPE;
    memcpy(rr->data,data,size);
}

//设置QNAME
void setQNAME(question *q,char *name){
    strncpy(q->q_name,name,MAX_LEN);
}

int check(int c){
    char a[] = {"!@#$%^&*()_+-=,./?'\"\\"};
    if(isalnum(c))
        return 1;
    for(int i=0;i<sizeof (a)-1;i++)
        if(c == a[i])
            return 1;
    return 0;
}

void showFlag(uint16_t flag){
    for(int i = 0;i < 16;i++)
        if((flag & (1 << i)) != 0)
            printf("1 ");
        else
            printf("0 ");
}

//调试用，打印一条报文
void showMsg(message *msg){
    char *type[29],*class[] = {NULL,"IN","CS","CH","HS"},*rr_t[] = {"ANSWER","AUTHORITY","ADDITIONAL"};
    type[1] = "A";type[2] = "NS";type[5] = "CNAME";type[28] = "AAAA";

    printf("ID:%04x  ",msg->ID);
    printf("flag:"); showFlag(msg->flag);printf("\n");
    printf("q_count:%d ans_count:%d auth_count:%d add_count:%d\n",msg->q_count,msg->RR_count[0],msg->RR_count[1],msg->RR_count[2]);
    printf("----------questions--------------\n");
    for(int i=0;i<msg->q_count;i++){
        printf("name:%s  type:%s  class:%s\n",msg->ques[i]->q_name,type[msg->ques[i]->q_type],class[msg->ques[i]->q_class]);
    }
    printf("--------------end of question ----------------\n");
    for(int i=0;i<3;i++){
        printf("-----------------%s---------------\n",rr_t[i]);
        for(int j=0;j < msg->RR_count[i];j++){
            if(msg->resourse_record[i][j]->data_type == STRING_TYPE)
                printf("name:%s  type:%s class:%s TTL:%u len:%u data:%s\n",
                   msg->resourse_record[i][j]->name,type[msg->resourse_record[i][j]->type],
                   class[msg->resourse_record[i][j]->class],msg->resourse_record[i][j]->TTL,msg->resourse_record[i][j]->data_length,msg->resourse_record[i][j]->string_data);
            else
                printf("name:%s  type:%s class:%s TTL:%u len:%u data:...\n",
                       msg->resourse_record[i][j]->name,type[msg->resourse_record[i][j]->type],
                       class[msg->resourse_record[i][j]->class],msg->resourse_record[i][j]->TTL,msg->resourse_record[i][j]->data_length);
        }
        printf("-----------------end of %s---------------\n",rr_t[i]);
    }
}

void showMem(void *mem,size_t len){
    char *p = mem,*tmp;
    while(((void*)p-mem) < len){
        tmp = p;
        printf("%04x  ", (unsigned) ((void *) p - mem));
        for(int i=0;i<16;i++){
            if(((void*)p-mem) < len){
                printf("%02x ", (uint32_t)*p & 0xff);
                p++;
            }else
                printf("!! ");
            if (i == 7)
                printf("  ");
        }
        printf("  ");
        for(int i=0;i<16;i++){
            //printf("in for loop:%d %c\n",*tmp);
            if(((void*)tmp-mem) < len){
                if(check(*tmp))
                    putchar(*tmp);
                else
                    putchar('.');
                tmp++;
            } else {
                putchar('?');
            }
            if(i == 7)
                printf("  ");
        }
        printf("\n");
    }
}
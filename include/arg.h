//
// Created by 谢卫凯 on 2022/5/26.
//

#ifndef DNS_ARG_H
#define DNS_ARG_H

#include "hash.h"

typedef struct {
    HashTab *ht;
}Arg;

// 依据命令行参数创建一个Arg并返回指向其的指针
Arg *NewArg(int argc,char **argv);

// 销毁Arg
void DestroyArg(Arg *arg);

// 根据key 返回 value字符串
char *getStr(Arg *arg,char *key);

// 根据key 返回value对应的int数
// 如命令行参数中若有 size=10
// 则getInt(arg,"size")返回10
int getInt(Arg *arg,char *key);

// 测试某个key对应的value是否和参数value一致
// 比如命令行参数为 debug=info
// 则match("debug","info")返回true
int matchArg(Arg *arg, char *key, char *value);

#endif //DNS_ARG_H

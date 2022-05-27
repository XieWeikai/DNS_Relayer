//
// Created by 谢卫凯 on 2022/5/26.
//

#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "clog.h"
#include "hash.h"


// 处理命令行参数
// 命令行参数形如 debug=info dns=192.168.43.1 这样的key=value对
// 也可以是单独的 key 如 help 这种写法等于写help=true
Arg *NewArg(int argc, char **argv) {
    Arg *ta = calloc(1, sizeof(Arg));

    log_check(ta != NULL,"fail to allocate memory for Arg");
    ta->ht = NewHashTab();

    char key[50];
    char value[50];
    char *p;
    char *delim = "=";
    for (int i = 1; i < argc; i++) {
        strncpy(key,strtok(argv[i],delim),50);
        if((p = strtok(NULL,delim)) == NULL)
            strncpy(value,"true",50);
        else
            strncpy(value,p,50);
        insert(ta->ht,key,value,strlen(value)+1);
    }

    return ta;
}

void DestroyArg(Arg *arg) {
    DestroyHashTab(arg->ht);
}

char *getStr(Arg *arg,char *key) {
    return search(arg->ht,key);
}

int getInt(Arg *arg, char *key) {
    char *p = search(arg->ht,key);
    if(p == NULL)
        return 0;
    return atoi(p);
}

int matchArg(Arg *arg, char *key, char *value) {
    char *p = search(arg->ht,key);
    return p !=NULL && strncmp(p,value,strlen(value)) == 0;
}



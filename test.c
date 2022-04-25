//
// Created by 谢卫凯 on 2022/4/25.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "thread_pool.h"

char *name[] = {"xwk","wql","yr","hr","yyj","ysfj"};
int count[6] = {0};

void sayHello(void *n){
    printf("Hello %s\n",(char *)n);
    for(int i=0;i<6;i++)
        if(strncmp(name[i],(char*)n, 4) == 0)
            count[i] ++;
}

int main(){
    Pool p = CreateThreadPool(100);
    for(int i=0;i<1000;i++){
        for(int j=0;j<6;j++)
            AddTask(p,sayHello,name[j]);
    }

    sleep(5);
    ClosePool(p);
    for(int i=0;i<6;i++)
        printf("count[%d] = %d\n",i,count[i]);

    return 0;
}
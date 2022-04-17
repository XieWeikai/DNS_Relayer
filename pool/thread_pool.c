//
// Created by 谢卫凯 on 2022/4/13.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "thread_pool.h"

typedef struct task{
    void (*func)(void *); // 待执行的函数
    void *arg; // 参数列表
}Task;

typedef struct threadpool{
    safequeue q;
}ThreadPool;

// 消费者 负责接收任务并执行
static void *worker(void *data){
    //printf("start worker\n");
    safequeue taskQue = data;
    Task *t;
    while((t = SafeDeque(taskQue)) != NULL){
        t->func(t->arg);
        free(t);
    }
    //printf("stop worker\n");
    return NULL;
}

// 创建一个有num个线程的线程池
ThreadPool *CreateThreadPool(int num){
    ThreadPool *pool = malloc(sizeof(ThreadPool));
    pool->q = NewSafeQueue();
    pthread_t tid;
    for(int i=0;i<num;i++)
        pthread_create(&tid,NULL,worker,pool->q);
    return pool;
}

// 添加一个任务交给线程池来做
void AddTask(ThreadPool *pool,void (*func)(void *),void *data){
    Task *t = malloc(sizeof(Task));
    t->func = func;
    t->arg = data;
    SafeEnque(pool->q,t);
}

// 关闭线程池
void ClosePool(ThreadPool *pool){
    SafeDestroy(pool->q);
    free(pool);
}
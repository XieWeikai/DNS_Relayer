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
    safequeue q;   // 任务队列
    pthread_t *ids; // 线程id
    int num; // 线程数
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
    pool->ids = calloc(num,sizeof(pthread_t));
    pool->num = num;
    pthread_t tid;
    //pthread_attr_t attr;
    for(int i=0;i<num;i++) {
        //pthread_attr_init(&attr);
        //pthread_attr_setdetachstate(&attr, 1); // 设置这个属性的线性不需要join 若用默认属性，则不join的线程的资源不会被回收也就是内存泄漏 线程池中的线程不需要join
        //pthread_create(&tid, &attr, worker, pool->q);
        //pthread_attr_destroy(&attr);
        pthread_create(&tid,NULL,worker,pool->q); // 还是创建需要join的线程吧，ClosePool时等待所有线程退出
        pool->ids[i] = tid;
    }
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
    SafeDestroy(pool->q);  // 摧毁任务队列 工作线程会开始结束
    for(int i=0;i<pool->num;i++)
        pthread_join(pool->ids[i],NULL); // 等待工作线程退出
    free(pool->ids);
    free(pool);
}
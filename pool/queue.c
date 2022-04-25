//
// Created by 谢卫凯 on 2022/4/13.
//
#include <pthread.h>
#include <stdlib.h>

#include "thread_pool.h"

typedef struct queue_node{
    void *data;
    struct queue_node *next;
}QueNode;

typedef struct safeque{
    QueNode *head,*tail;
    pthread_mutex_t q_lock;
    pthread_cond_t q_ready;
    int destroyed;
    int usedNum; // 正在用这个队列的数目  销毁队列时要确保没有用这个队列的线程存在
    pthread_mutex_t numMut;// 锁usedNum的锁
    pthread_cond_t noUsed; // 没有用该队列的线程
}SafeQueue;

SafeQueue *NewSafeQueue(){ // 不考虑内存分配失败
    SafeQueue *q = malloc(sizeof(*q));
    q->head = q->tail = malloc(sizeof(QueNode)); // 头结点，不使用的
    q->head->next = NULL;
    pthread_mutex_init(&q->q_lock,NULL);
    pthread_cond_init(&q->q_ready,NULL);
    pthread_mutex_init(&q->numMut,NULL);
    pthread_cond_init(&q->noUsed,NULL);
    q->usedNum = 0;
    q->destroyed = 0;
    return q;
}

void SafeEnque(SafeQueue *q,void *data){
    if(q->destroyed)
        return;
    pthread_mutex_lock(&q->numMut);
    q->usedNum ++;
    pthread_mutex_unlock(&q->numMut);

    QueNode *node = malloc(sizeof(QueNode));
    node->data = data;
    node->next = NULL;
    pthread_mutex_lock(&q->q_lock);
    if(q->destroyed){
        free(data);
        pthread_mutex_unlock(&q->q_lock);
        return;
    }
    q->tail->next = node;
    q->tail = node;
    pthread_mutex_unlock(&q->q_lock);
    pthread_cond_signal(&q->q_ready);

    pthread_mutex_lock(&q->numMut);
    q->usedNum --;
    pthread_cond_signal(&q->noUsed);
    pthread_mutex_unlock(&q->numMut);
}

void *SafeDeque(SafeQueue *q){
    void *data;
    QueNode *t;
    if(q->destroyed)
        return NULL;
    pthread_mutex_lock(&q->numMut);
    q->usedNum ++;
    pthread_mutex_unlock(&q->numMut);

    pthread_mutex_lock(&q->q_lock);
    while(!q->destroyed && q->head == q->tail)
        pthread_cond_wait(&q->q_ready,&q->q_lock);
    if(q->destroyed) {
        pthread_mutex_unlock(&q->q_lock);

        pthread_mutex_lock(&q->numMut);
        q->usedNum --;
        pthread_cond_signal(&q->noUsed);
        pthread_mutex_unlock(&q->numMut);
        return NULL;
    }
    t = q->head->next;
    data = t->data;
    q->head->next = t->next;
    if(t == q->tail)
        q->tail = q->head;
    pthread_mutex_unlock(&q->q_lock);
    free(t);

    pthread_mutex_lock(&q->numMut);
    q->usedNum --;
    pthread_cond_signal(&q->noUsed);
    pthread_mutex_unlock(&q->numMut);

    return data;
}

void SafeDestroy(SafeQueue *q){
    q->destroyed = 1;
    pthread_cond_broadcast(&q->q_ready);

    pthread_mutex_lock(&q->numMut);
    while(q->usedNum > 0)
        pthread_cond_wait(&q->noUsed,&q->numMut);
    pthread_mutex_unlock(&q->numMut);

    QueNode *next ,*t = NULL;
    for(t=q->head;t != NULL;t=next) {
        next = t->next;
        free(t);
    }
    pthread_mutex_destroy(&q->q_lock);
    pthread_cond_destroy(&q->q_ready);
    pthread_mutex_destroy(&q->numMut);
    pthread_cond_destroy(&q->noUsed);

    // free(q);  注意这里的free(q); 有可能会导致接下来调用的入队出队会访问不该访问的内存，因此这个free(q)就交给队列的拥有者去做了
}
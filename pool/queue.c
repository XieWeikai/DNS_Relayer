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
}SafeQueue;

SafeQueue *NewSafeQueue(){ // 不考虑内存分配失败
    SafeQueue *q = malloc(sizeof(*q));
    q->head = q->tail = malloc(sizeof(QueNode)); // 头结点，不使用的
    q->head->next = NULL;
    pthread_mutex_init(&q->q_lock,NULL);
    pthread_cond_init(&q->q_ready,NULL);
    q->destroyed = 0;
    return q;
}

void SafeEnque(SafeQueue *q,void *data){
    if(q->destroyed)
        return;

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
    pthread_cond_signal(&q->q_ready);
    pthread_mutex_unlock(&q->q_lock);
}

void *SafeDeque(SafeQueue *q){
    void *data;
    QueNode *t;
    if(q->destroyed)
        return NULL;

    pthread_mutex_lock(&q->q_lock);
    while(!q->destroyed && q->head == q->tail)
        pthread_cond_wait(&q->q_ready,&q->q_lock);

    if(q->destroyed) {
        pthread_mutex_unlock(&q->q_lock);
        return NULL;
    }
    t = q->head->next;
    data = t->data;
    q->head->next = t->next;
    if(t == q->tail)
        q->tail = q->head;
    pthread_mutex_unlock(&q->q_lock);
    free(t);

    return data;
}

// 这个函数不会释放q的空间，需要使用者自行释放
// 关闭队列
void SafeClose(SafeQueue *q){
    pthread_mutex_lock(&q->q_lock);
    q->destroyed = 1;
    pthread_cond_broadcast(&q->q_ready);

    QueNode *next ,*t = NULL;
    for(t=q->head;t != NULL;t=next) {
        next = t->next;
        free(t);
    }
    pthread_mutex_unlock(&q->q_lock);
}

// 彻底销毁该队列
void SafeDestroy(SafeQueue *q){
    pthread_mutex_destroy(&q->q_lock);
    pthread_cond_destroy(&q->q_ready);
    free(q);
}

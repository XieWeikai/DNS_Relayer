//
// Created by 谢卫凯 on 2022/4/16.
//

#ifndef DNS_CACHE_H
#define DNS_CACHE_H

#include <pthread.h>

#include "linkList.h"
#include "hash.h"

typedef struct cache {
    HashTab *hTab;   // 哈希表 哈希表中存的是指向双向链表节点的指针
    LinkList *lList; // 双向链表
    int len; // 已经存入的个数
    int maxSize; // 最大缓存个数
    pthread_rwlock_t mux; // 多读单写锁

    void *(*copy)(void *data); // 复制函数，用于复制一份要存入的数据
    void (*delete)(void *data); // 销毁函数，用于销毁一份存入的数据
} Cache;

// 创建一个最多存放maxSize个条目的缓存
// 传入的copy 和 delete可以为NULL
// 为NULL 则执行默认操作
Cache *CreateCache(int maxSize,void *(*copy)(void *data),void (*delete)(void *data));

// 销毁缓存
void DestroyCache(Cache *c);

// 将一个条目存入缓存中
// c 为先前创建的缓存 key为键 data为指向想要缓存数据的指针 size为要缓存数据的大小
// TTL为该缓存条目存活时间（单位：秒）（这个时间不是指未使用该缓存的时间，而是指put进来之后能存活的时间）
// 若key与之前存放条目相同会覆盖之前的条目
void CachePut(Cache *c, char *key, void *data, size_t size, time_t TTL);

// 从缓存中取出一条数据
// c 为缓存 key为键
// 返回指向数据的指针
// 注意返回的指针指向的空间要由使用者自行释放！！！！（为了防止并发操作带来的冲突 所以返回的数据是额外拷贝的一份 交由使用者管理 故需要使用者释放）
void *CacheGet(Cache *c, char *key);

#endif //DNS_CACHE_H

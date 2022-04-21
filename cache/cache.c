//
// Created by 谢卫凯 on 2022/4/16.
//
#include <memory.h>
#include <string.h>
#include <time.h>

#include "cache.h"

#define SAVE_UNIT_KEY_MAX_LEN 50

// 双向链表里存的就是这玩意
typedef struct {
    char key[SAVE_UNIT_KEY_MAX_LEN+1];
    time_t saveTime; //存入缓存的时间
    time_t TTL;   // 存活时间
    void *data;  // 数据
    size_t dataSize;
}saveUnit;

Cache *CreateCache(int maxSize){
    Cache *c = malloc(sizeof(Cache));
    c->hTab = NewHashTab();
    c->lList = NewLinkList();
    pthread_rwlock_init(&c->mux,NULL);
//    pthread_mutex_init(&c->mu,NULL);
    c->maxSize = maxSize;
    c->len = 0;
    return c;
}

void destroySavaUnit(void *date){
    saveUnit *ui = date;
    free(ui->data);
}

void DestroyCache(Cache *c) {
    DestroyHashTab(c->hTab);
    DestroyLinkList(c->lList,destroySavaUnit);
    pthread_rwlock_destroy(&c->mux);
    free(c);
}

void CachePut(Cache *c,char *key, void *data, size_t size, time_t TTL) {
    saveUnit ui;
    saveUnit *pui;
    linkNode *lkn,**tmp;
    pthread_rwlock_wrlock(&c->mux);
    tmp = search(c->hTab,key);
    if(tmp != NULL){ //已经有了，那就更新
        lkn = *tmp;
        pui = lkn->data;
        free(pui->data);
        pui->data = malloc(size);
        memcpy(pui->data,data,size);
        pui->saveTime = time(NULL);
        pui->TTL = TTL;
        pui->dataSize = size;
        // 更新后放到头部
        RemoveFromLinkList(c->lList,lkn);
        PutInHead(c->lList,lkn);
        // 不要忘记解锁
        pthread_rwlock_unlock(&c->mux);
        return;
    }

    if(c->len >= c->maxSize){ // 删除尾部的缓存 腾出空间
        lkn = GetLastLinkNode(c->lList);
        pui = lkn->data;
        delete(c->hTab,pui->key); // 从哈希表中移除
        RemoveFromLinkList(c->lList,lkn); // 从双向链表中移除
        free(pui->data); //释放缓存的数据
        freeLinkNode(lkn); // 释放该链表节点
        c->len -- ;
    }
    ui.TTL = TTL;
    ui.data = malloc(size);
    strncpy(ui.key,key,SAVE_UNIT_KEY_MAX_LEN+1);
    memcpy(ui.data,data,size);
    ui.dataSize = size;
    ui.saveTime = time(NULL);
    linkNode *nod;
    nod = PutInHead(c->lList,newLinkNode(&ui, sizeof(ui)));
    insert(c->hTab,key,&nod, sizeof(linkNode*)); // 注意！！！ 哈希表里存的是指向linkNode的指针
    c->len++;
    pthread_rwlock_unlock(&c->mux);
}

// 返回一份拷贝的内存，释放由接受者来释放
// 如果直接返回指向存在缓存中的数据的指针
// 有可能在使用这块空间时
// 该空间因为别处的调用而被改变了
void *CacheGet(Cache *c, char *key) {
    saveUnit *ui;
    linkNode **t,*lkn;
    void *res;
    pthread_rwlock_wrlock(&c->mux); // 查看搜索的键是否要删除，删除是写操作 上写锁
    if((t = search(c->hTab,key)) == NULL){
        pthread_rwlock_unlock(&c->mux);
        return NULL;
    }
    lkn = *t;
    ui = lkn->data;
    if(time(NULL)-ui->saveTime > ui->TTL){ // 超出TTL 该缓存不可用了 删除
        delete(c->hTab,ui->key);
        free(ui->data);
        RemoveFromLinkList(c->lList,lkn);
        freeLinkNode(lkn);
        pthread_rwlock_unlock(&c->mux);
        c->len --;
        return NULL;
    }
    // 移到开头
    RemoveFromLinkList(c->lList,lkn);
    PutInHead(c->lList,lkn);
    pthread_rwlock_unlock(&c->mux);

    pthread_rwlock_rdlock(&c->mux); // 这种操作是并发安全的 上读锁
    res = malloc(ui->dataSize);
    memcpy(res,ui->data,ui->dataSize);
    pthread_rwlock_unlock(&c->mux);
    return res;
}

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
    size_t dataSize; // 数据大小
}saveUnit;

Cache *CreateCache(int maxSize,void *(*copy)(void *data),void (*delete)(void *data)){
    Cache *c = malloc(sizeof(Cache));
    c->hTab = NewHashTab();
    c->lList = NewLinkList();
    pthread_rwlock_init(&c->mux,NULL);
//    pthread_mutex_init(&c->mu,NULL);
    c->maxSize = maxSize;
    c->len = 0;
    c->copy = copy;
    c->delete = delete;
    return c;
}

static pthread_mutex_t deleteDataMut = PTHREAD_MUTEX_INITIALIZER; // 用于锁定下面的deleteData
static void (*deleteData)(void *) = NULL;

void destroySavaUnit(void *date){
    saveUnit *ui = date;
    if(deleteData != NULL)
        deleteData(ui->data);
    else
        free(ui->data);
}

void DestroyCache(Cache *c) {
    DestroyHashTab(c->hTab);

    pthread_mutex_lock(&deleteDataMut);
    if(c->delete != NULL)
        deleteData = c->delete; // 设置销毁缓存数据的方式
    DestroyLinkList(c->lList,destroySavaUnit);
    deleteData = NULL;
    pthread_mutex_unlock(&deleteDataMut);

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
        if(c->delete == NULL) // 默认的删除数据方式
            free(pui->data);
        else
            c->delete(pui->data); //自定义的删除数据方式

        if(c->copy == NULL){ // 默认的构造数据方式
            pui->data = malloc(size);
            memcpy(pui->data, data, size);
        }else
            pui->data = c->copy(data); // 自定义的构造数据
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
        if(c->delete == NULL) //默认删除数据方式
            free(pui->data); //释放缓存的数据
        else
            c->delete(pui->data);
        freeLinkNode(lkn); // 释放该链表节点
        c->len -- ;
    }

    ui.TTL = TTL;
    if(c->copy == NULL){ // 默认的复制存储方式
        ui.data = malloc(size);
        memcpy(ui.data, data, size);
    }else
        ui.data = c->copy(data); // 使用者自定义的复制方式

    strncpy(ui.key, key, SAVE_UNIT_KEY_MAX_LEN + 1);
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
// 没查到则返回NULL
void *CacheGet(Cache *c, char *key) {
    saveUnit *ui;
    linkNode **t,*lkn;
    void *res;
    pthread_rwlock_wrlock(&c->mux); // 查看搜索的键是否要删除，删除是写操作 上写锁
    if((t = search(c->hTab,key)) == NULL){ // 没查到
        pthread_rwlock_unlock(&c->mux);
        return NULL;
    }
    lkn = *t;
    ui = lkn->data;
    if(time(NULL)-ui->saveTime > ui->TTL){ // 超出TTL 该缓存不可用了 删除
        delete(c->hTab,ui->key);
        if(c->delete == NULL) // 默认销毁数据方式
            free(ui->data);
        else
            c->delete(ui->data);
        RemoveFromLinkList(c->lList,lkn);
        freeLinkNode(lkn);
        c->len --;
        pthread_rwlock_unlock(&c->mux);
        return NULL;
    }
    // 移到开头
    RemoveFromLinkList(c->lList,lkn);
    PutInHead(c->lList,lkn);
    pthread_rwlock_unlock(&c->mux);

    pthread_rwlock_rdlock(&c->mux); // 这种操作是并发安全的 上读锁

    if(c->copy == NULL) {
        res = malloc(ui->dataSize);
        memcpy(res, ui->data, ui->dataSize);
    }else
        res = c->copy(ui->data);
    pthread_rwlock_unlock(&c->mux);
    return res;
}

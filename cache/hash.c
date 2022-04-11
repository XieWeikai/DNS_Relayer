//
// Created by 谢卫凯 on 2022/4/3.
//
#include <stdlib.h>
#include <string.h>

#include "hash.h"

#define INIT_SIZE 1

// 新建哈希表
HashTab *NewHashTab(){
    HashTab *t = malloc(sizeof (*t));
    if(!t)
        return NULL;
    t->h = calloc(INIT_SIZE , sizeof (Node *));
    t->len = INIT_SIZE;
    t->num = 0;
    return t;
}

//销毁哈希表
void DestroyHashTab(HashTab *h){
    Node *next;
    for(int i=0;i<h->len;i++)
        for(Node *t=h->h[i];t != NULL;t = next) {
            next = t->next;
            free(t->data);
        }
    free(h);
}

// 辅助函数，新建节点
static Node *newNode(char *name,void *data){
    Node *t = malloc(sizeof (*t));
    strncpy(t->name,name,MAX_LEN);
    t->data = data;
    t->next = NULL;
    return t;
}

// 随便写的计算字符串哈希值的哈希函数
static unsigned int hash(char *name){
    unsigned int res = 0;
    while(*name)
        res += (*name++)*39;
    return res;
}

// 改变哈希表坑位大小的辅助函数
static void rescale(HashTab *ht,size_t size){
    Node **h = calloc(size,sizeof(Node *));
    Node *next;
    unsigned int hs;
    for(int i=0;i<ht->len;i++){
        for(Node *t=ht->h[i];t != NULL;t = next){
            next = t->next;
            hs = hash(t->name) % size;
            t->next = h[hs];
            h[hs] = t;
        }
    }
    free(ht->h);
    ht->h = h;
    ht->len = size;
}

// 插入，插入时会将数据复制一份，故要有数据大小
// 若该name已存在则修改数据
void insert(HashTab *ht,char *name,void *data,size_t size){
    if(ht == NULL)
        return;
    void *t = malloc(size);
    memcpy(t,data,size);
    unsigned int ha = hash(name) % ht->len;
    for(Node *tmp = ht->h[ha];tmp != NULL;tmp=tmp->next){
        if(strcmp(tmp->name,name) == 0){
            free(tmp->data);
            tmp->data = t;
            return;
        }
    }
    Node *n = newNode(name,t);
    n->next = ht->h[ha];
    ht->h[ha] = n;
    ht->num ++;
    if(ht->num >= ht->len * 2)
        rescale(ht,ht->len<<1);
}

// 查找数据，无则返回空
void *search(HashTab *ht,char *name){
    if(ht == NULL)
        return NULL;
    unsigned int hs = hash(name) % ht->len;
    for(Node *t = ht->h[hs];t != NULL;t = t->next){
        if(strcmp(t->name,name) == 0)
            return t->data;
    }
    return NULL;
}

// 辅助函数，从链表里删除节点
static Node *deleteNode(Node *head,char *name){
    Node *pre = NULL,*cur = head;
    for(;cur != NULL && strcmp(cur->name,name) != 0;pre = cur,cur = cur->next)
        ;
    if(cur == NULL)
        return head;
    if(pre == NULL)
        head = cur->next;
    else
        pre->next = cur->next;
    free(cur->data);
    free(cur);
    return head;
}

// 删除一个数据，没有这个数据则啥也不干
void delete(HashTab *ht,char *name){
    if(ht == NULL)
        return;
    unsigned int hs = hash(name) % ht->len;
    ht->h[hs] = deleteNode(ht->h[hs],name);
    ht->num--;
    if(ht->num * 2 < ht->len)
        rescale(ht,ht->len>>1);
}

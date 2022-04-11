//
// Created by 谢卫凯 on 2022/4/3.
//

#ifndef DNS_HASH_H
#define DNS_HASH_H

#ifndef MAX_LEN
#define MAX_LEN 32
#endif

typedef struct node{
    char name[MAX_LEN]; //键
    void *data;         //值
    struct node *next;  //防冲突
}Node;

typedef struct {
    Node **h; //表
    unsigned int len; // h的坑位
    unsigned int num; // 表中元素个数
}HashTab;

// 新建哈希表
HashTab *NewHashTab();

//销毁哈希表
void DestroyHashTab(HashTab *h);

// 插入，插入时会将数据复制一份，故要有数据大小
// 若传入NULL则啥也不干
void insert(HashTab *ht,char *name,void *data,size_t size);

// 查找数据，无则返回空
// 若传入NULL则啥也不干
void *search(HashTab *ht,char *name);

// 删除一个数据，没有这个数据则啥也不干
// 若传入NULL则啥也不干
void delete(HashTab *ht,char *name);

#endif //DNS_HASH_H

//
// Created by 谢卫凯 on 2022/4/16.
//

#ifndef DNS_LINKLIST_H
#define DNS_LINKLIST_H

#include <stdlib.h>

typedef struct linknode{
    void *data; // 数据
    struct linknode *pre,*next; // 无需多言
}linkNode;

// 双向链表
typedef struct link{
    linkNode *head,*tail; //头结点尾节点都不使用
}LinkList;

// 创建新节点 将数据复制一份
linkNode *newLinkNode(void *data,size_t size);

// 清除一个节点
void freeLinkNode(linkNode *nod);

// 新链表
LinkList *NewLinkList();

// 销毁链表
// 链表内data域内可能也需要额外处理，可以通过传destroyFunc
// 在释放data前，先调用destroyFunc(data)
// 若传NULL则不做处理
void DestroyLinkList(LinkList *l,void (*destroyFunc)(void *));

// 放入头部,返回指向放入节点的指针
linkNode *PutInHead(LinkList *l,linkNode *nod);

// 放入尾部
linkNode *PutInTail(LinkList *l,linkNode *nod);

// 取出第一个节点 没有则返回NULL
linkNode *GetFirstLinkNode(LinkList *l);

// 取出最后一个节点 没有则返回NULL
linkNode *GetLastLinkNode(LinkList *l);

// 移除出链表（但该节点还没有删）只是从链表中移出去了
void RemoveFromLinkList(LinkList *l,linkNode *nod);

#endif //DNS_LINKLIST_H

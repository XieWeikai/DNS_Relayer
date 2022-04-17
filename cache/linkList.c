//
// Created by 谢卫凯 on 2022/4/16.
//

#include <stdlib.h>
#include <memory.h>

#include "linkList.h"

linkNode *newLinkNode(void *data, size_t size) {
    linkNode *res = malloc(sizeof(linkNode));
    res->next = res->pre = NULL;
    res->data = malloc(size);
    memcpy(res->data, data, size);

    return res;
}

void freeLinkNode(linkNode *nod) {
    free(nod->data);
    free(nod);
}

LinkList *NewLinkList() {
    LinkList *res = malloc(sizeof(LinkList));
    res->head = res->tail = malloc(sizeof(linkNode)); // 头尾共用
    res->head->next = res->head->pre = res->head;
    return res;
}

linkNode *PutInHead(LinkList *l, linkNode *nod) {
    nod->next = l->head->next;
    l->head->next->pre = nod;
    l->head->next = nod;
    nod->pre = l->head;
    return nod;
}

linkNode *PutInTail(LinkList *l, linkNode *nod) {
    nod->pre = l->tail->pre;
    l->tail->pre->next = nod;
    nod->next = l->tail;
    l->tail->pre = nod;
    return nod;
}

void RemoveFromLinkList(LinkList *l, linkNode *nod) {
    nod->pre->next = nod->next;
    nod->next->pre = nod->pre;
    nod->next = nod->pre = NULL;
}

void DestroyLinkList(LinkList *l) {
    linkNode *t = l->head->next, *next = NULL;
    while (t != l->tail) {
        next = t->next;
        free(t->data);
        free(t);
        t = next;
    }
    free(l->head);
    free(l);
}

linkNode *GetFirstLinkNode(LinkList *l) {
    return l->head->next == l->tail ? NULL : l->head->next;
}

linkNode *GetLastLinkNode(LinkList *l) {
    return l->head->next == l->tail ? NULL : l->tail->pre;
}

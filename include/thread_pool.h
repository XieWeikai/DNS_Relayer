//
// Created by 谢卫凯 on 2022/4/13.
//

#ifndef DNS_THREAD_POOL_H
#define DNS_THREAD_POOL_H

// 一个并发安全的队列
typedef struct safeque *safequeue;

// 新建队列
safequeue NewSafeQueue();

// 入队
void SafeEnque(safequeue q, void *data);

// 出队
void *SafeDeque(safequeue q);

// 关闭该队列
// 释放一部分资源
// 关闭后依旧可以进行SafeEnque SafeDeque
// 但不会有任何效果
void SafeClose(safequeue q);

// 销毁队列，彻底释放掉该队列的全部占用资源
// 此后不得再对该队列有任何操作
// 否则会操作错误的内存
void SafeDestroy(safequeue q);

// 线程池指针
typedef struct threadpool *Pool;

// 创建一个有num个线程的线程池
Pool CreateThreadPool(int num);

// 添加一个任务交给线程池来做
void AddTask(Pool pool,void (*func)(void *),void *data);

// 关闭线程池
void ClosePool(Pool pool);

#endif //DNS_THREAD_POOL_H

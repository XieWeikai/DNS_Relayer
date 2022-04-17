# Thread_pool

[toc]

## 使用线程池

`thread_pool.c`实现了一个简单的线程池，`thread_pool.h`中暴露其接口，如下

```c
// 线程池指针
typedef struct threadpool *Pool;

// 创建一个有num个线程的线程池
Pool CreateThreadPool(int num);

// 添加一个任务交给线程池来做
// func为要执行的函数
// data为要传给func的参数
void AddTask(Pool pool,void (*func)(void *),void *data);

// 关闭线程池
void ClosePool(Pool pool);
```

线程池避免了不断创建并销毁线程的过程，只需要将任务分配给线程池中的线程即可，使用示例如下

```c
#include <stdio.h>
#include <unistd.h>

#include "thread_pool.h"

void func1(void *data){
    for(int i=0;i<5;i++) {
        printf("func1 !!!\n");
        sleep(1);
    }
}

void func2(void *data){
    for(int i=0;i<5;i++) {
        printf("func2 !!!\n");
        sleep(1);
    }
}

void func3(void *data){
    for(int i=0;i<3;i++) {
        printf("func3 !!!\n");
        sleep(1);
    }
}

int main(){
    Pool p = CreateThreadPool(2); // 两个线程的线程池
    AddTask(p,func1,NULL);
    AddTask(p,func2,NULL);
    AddTask(p,func3,NULL);

    sleep(8);
    ClosePool(p);
    return 0;
}
```

输出如下

```bash
func2 !!!
func1 !!!
func1 !!!
func2 !!!
func1 !!!
func2 !!!
func2 !!!
func1 !!!
func2 !!!
func1 !!!
func3 !!!
func3 !!!
func3 !!!
```

可以看到`func3`等到了最后才执行这是因为线程池中只有两个线程，得先执行完一个任务才能空出线程池接着执行`func3`。
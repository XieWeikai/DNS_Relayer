# DNS relayer

[toc]

## 说明

该项目为北邮计算机网络课程设计...目标是实现一个DNS中继器。

---

## 设计

![design](./doc/img/design.png)

目前的设计如上图。

---

## 目前项目结构及目前实现的功能

```bash
.
├── CMakeLists.txt
├── cache
│   ├── cache.c
│   ├── hash.c
│   └── linkList.c
├── cli.c
├── doc
│   ├── Cache.md
│   ├── DNS_message.md
│   └── Thread_pool.md
├── include
│   ├── cache.h
│   ├── hash.h
│   ├── linkList.h
│   ├── message.h
│   ├── thread_pool.h
│   └── wrap.h
├── message
│   └── message.c
├── pool
│   ├── queue.c
│   └── thread_pool.c
├── readme.md
├── server.c
└── tool
    └── wrap.c
```

目前的目录结构如上，其中`cli.c`实现了一个简单的类似于`nslookup`的小工具，可以通过`cli.c`看看如何实现DNS报文编码解码。`server.c`实现了一个DNS中继器（不带缓存，不带本地资源记录），要尝试使用只需要将`server.c`中的如下宏修改为你的`local DNS Server`即可

```c
#define DNS_IP "192.168.43.1"  // 这个ip 是我这的local DNS server的ip 
```

---

`tool`目录下给一些网络相关的函数包了一层皮，详见[基于tcp协议的网络程序](https://www.bookstack.cn/read/linux-c/fa2e4c92668c4bb7.md)。

`message`目录下实现了和DNS报文相关的功能，详见`doc/DNS_message.md`。

`pool`目录实现了一个简单的线程池，详见`doc/Thread_pool.md`。

`cache`目录实现了缓存功能，详见`doc/Cache.md`。

`include`包含各模块功能的接口头文件。

`doc`内为各模块文档。

---

## 尚待完成的部分

目前距离最终版本仍差以下部分

- 主线程的逻辑实现
- 处理用户请求线程的逻辑实现
- 本地资源记录（即在本地的文件中查找资源记录）



## 目标

### 基础目标

至少能对用户的A记录查询要求做出响应。

### 感觉较容易实现的额外目标

对用户的CNAME、AAAA、NS记录查询做出响应。
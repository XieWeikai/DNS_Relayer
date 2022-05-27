

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
├── dnsrelay.txt
├── doc
│   ├── Arg.md
│   ├── Cache.md
│   ├── DNS_message.md
│   └── Thread_pool.md
├── include
│   ├── arg.h
│   ├── cache.h
│   ├── clog.h
│   ├── file_reader.h
│   ├── hash.h
│   ├── linkList.h
│   ├── message.h
│   ├── thread_pool.h
│   └── wrap.h
├── man.txt
├── message
│   └── message.c
├── pool
│   ├── queue.c
│   └── thread_pool.c
├── readme.md
├── relayer.c
├── server.c
├── test
│   └── file_reader_test.c
├── test.c
└── tool
    ├── arg.c
    ├── clog.c
    ├── file_reader.c
    └── wrap.c
```

目前的目录结构如上，其中`cli.c`实现了一个简单的类似于`nslookup`的小工具，可以通过`cli.c`看看如何实现DNS报文编码解码。`server.c`实现了一个DNS中继器（不带缓存，不带本地资源记录），要尝试使用只需要将`server.c`中的如下宏修改为你的`local DNS Server`即可

```c
#define DNS_IP "192.168.43.1"  // 这个ip 是我这的local DNS server的ip 
```



`test.c`和`test`目录下是测试时残留下来的文件。`relayer.c`是一个单纯的中继器，没有中间处理程。`server.c`为该课设的主程序。`dnsrelay.txt`为记录域名及ip的文本文件，`man.txt`需要放到程序执行的同级目录下，否则程序无法正常输出帮助信息。

---

## 各目录实现的内容

`tool`：

- `wrap.c`给一些网络相关的函数包了一层皮，详见:[基于tcp协议的网络程序](https://www.bookstack.cn/read/linux-c/fa2e4c92668c4bb7.md)。
- `arg.c`实现了处理命令行程序的小工具。
- `file_reader.c`实现了从给定的`relayer.txt`中读出指定域名的ipv4地址功能。
- `clog.c`实现了一个轻量级的日志功能。

`message`目录下实现了和DNS报文相关的功能，详见`doc/DNS_message.md`。

`pool`目录实现了一个简单的线程池，详见`doc/Thread_pool.md`。

`cache`目录实现了缓存功能，详见`doc/Cache.md`。

`include`包含各模块功能的接口头文件。

`doc`内为各模块文档。

---

## ~~尚待完成的部分~~

~~目前距离最终版本仍差以下部分~~

- ~~主线程的逻辑实现~~
- ~~处理用户请求线程的逻辑实现~~
- ~~本地资源记录（即在本地的文件中查找资源记录）~~



## 目标

### 基础目标

至少能对用户的A记录查询要求做出响应。

### 感觉较容易实现的额外目标

对用户的CNAME、AAAA、NS记录查询做出响应。



---

## 现在的成果

### 生成可执行文件

示例命令如下：

```bash
git clone git@github.com:XieWeikai/DNS_Relayer.git
mkdir build
cd build
cmake ../DNS_Relayer
make
```

build目录下生成一系列可执行文件，其中`server`为需要的程序。将`man.txt`和`dnsrelay.txt`复制到build目录下，执行如下命令启动DNS中继服务器,其中`dns=...`参数换成你家的本地DNS服务器ip地址。

```bash
./server localfile=dnsrelay.txt dns=192.168.43.1
```

### 目前程序功能

#### 缓存

程序会自动缓存如下类型资源记录方便查找

- A
- NS
- CNAME

其余类型不做缓存，对于一个域名，缓存可以保存多条A记录或多条NS记录。

`cache.c`中实现的缓存功能查找时间复杂度为`O(1)`,基于哈希表和双向链表，该缓存实现是并发安全的。缓存淘汰策略有二：最久不访问的记录淘汰；超过存入缓存时设定的超时时间时淘汰。

#### 文件查找

目前实现了从文本文件中查找A类型资源记录的功能，程序为文件创建了索引，查找复杂度为`O(1)`.若缓存中找不到客户端查询的A类型资源，会从文件中查找，若查到则会将该记录放入缓存，方便下次查找。

#### 中继

若缓存中和文件中都没有记录，程序会像DNS服务器发送请求，并将请求响应回复给客户端，同时将响应中A、NS、CNAME记录送入缓存。



程序自带帮助如下~~（忽略蹩脚英语）~~

```
usage:
    server localfile=filename dns=local_dns_server_ip [options]

    local_dns_server_ip is ip of your Local DNS server.When our program work as a relayer,it will send
    message to the local_dns_server_ip.

    filename is the file that stores a series of domain names and their ipv4 addresses.Each line is in
    the form of: ip domain name.
    e.g.  dnsrelay.txt
    ...
    74.125.207.113 i2.ytimg.com
    74.125.207.113 i3.ytimg.com
    ...

options:
    debug=level
        level can be one of trace,debug,info,warn,error and fatal.Default level is info.

    poolsize=num_of_thread_pool
        Set the size of thread pool. Default size is 10.Size should not be more than 20.


    cachesize=size
        Set the size of cache, Default is 1024. Size should not be more than 2048.

    port=portnumber
        Set the listening port number to portnumber. Default is 53.

    --help
        Show this content.

```

---

## 题外话

程序中出现的英文若有任何错误，请忽视，意会即可......

程序中很多部分的代码因为设想的功能复杂不断改进，中途不免出现很多bug且不断小修小补，导致某些代码繁杂，且还留有debug痕迹......

主程序只是将其余各个部分的功能拼起来并实现程序需求，较为冗长无味。
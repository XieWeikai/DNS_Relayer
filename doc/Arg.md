# Arg

源程序中`arg.c`和`arg.h`两个文件提供了和命令行参数处理相关的功能，其接口如下

```c
// 依据命令行参数创建一个Arg并返回指向其的指针
// 处理命令行参数
// 命令行参数形如 debug=info dns=192.168.43.1 这样的key=value对
// 也可以是单独的 key 如 help 这种写法等于写help=true
Arg *NewArg(int argc,char **argv);

// 销毁Arg
void DestroyArg(Arg *arg);

// 根据key 返回 value字符串
char *getStr(Arg *arg,char *key);

// 根据key 返回value对应的int数
// 如命令行参数中若有 size=10
// 则getInt(arg,"size")返回10
int getInt(Arg *arg,char *key);

// 测试某个key对应的value是否和参数value一致
// 比如命令行参数为 debug=info
// 则match("debug","info")返回true
int matchArg(Arg *arg, char *key, char *value);
```


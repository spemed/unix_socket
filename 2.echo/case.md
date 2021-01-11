# 测试四种异常情况
## accept
+ 1.服务端已经打开了被动套接字,客户端使用connect完成三次握手后。在服务端accept之前使用close断开连接。测试一下accept的行为
```c
    int listen_fd = Server(SERVER_PORT);
        if (listen_fd < 0) {
            ERR_PRINT("failed to listen")
        }
        int pid;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        bzero(&client_addr,client_addr_len);
        sleep(10);
        while (1) {
            int connect_fd = accept(listen_fd,(SA*)&client_addr,&client_addr_len);
            //do xx
        }
```
+ 在进死循环之前,sleep 10s。给客户端留了10s的时间close套接字。结果是accept仍然可以从已完成三次握手的队列中取出已连接套接字。但是服务端的tcp状态机为close_wait。调用read系统调用时会返回0[读取到EOF,说明tcp客户端发送了fin包]
+ 2.服务端已经打开了被动套接字，客户端使用connect完成三次握手后。在服务端accept之前发送RST分节重置tcp链接
+ 待测试

## 连接过程中杀死服务端进程
### 正常流程
+ 1.客户端connect到服务端时，服务端会fork出一个子进程维护tcp连接[套接字]
+ 2.客户端阻塞于获取标准输入流[等待敲击键盘和回车]的系统调用
+ 3.使用kill命令杀死服务端子进程,进程在退出时会关闭所有资源,给套接字对应的tcp发送fin包
+ 4.客户端仍然阻塞于第二步的系统调用，但是fin包到达客户端时，客户端的tcp状态机将变成close_wait
+ 5.此时我们键入文字内容，客户端进程将执行write系统调用，把字节写入内核的发送缓冲区后返回，当tcp分节到达对端时，因为服务端的已连接套接字随着子进程被杀死而被释放，所以服务端将会返回RST分节。
+ 6.客户端进程调用read系统调用，由于此时tcp状态机处于close_wait状态，read返回0，退出事件循环并关闭客户端进程。
### 异常测试
+ 综上所述,分歧点主要是发生在第5和第6点。在第5步执行完成后，客户端虽然终将收到来自服务端的RST分节，但是网络的传输总需要时间，write系统调用不是等待tcp分节传输到对端后返回，而是只要把进程缓冲区的数据写入内核发送缓存区后就返回。所以在客户端的tcp收到RST分节之前，客户端进程就因为对close_wait状态的套接字执行read触发业务代码自动退出了。所以我们将会在write和read之间增加sleep函数，观察一下会发生什么有趣的事情
### 结论
+ 读一个已经接受了RST分节的套接字会报错 failed to read: Connection reset by peer [连接被重置]
+ 写一个已经接受了RST分节的套接字 [会收到一个SIGPIPE信号,默认行为是导致进程以code 13退出]
+ 如果我们注册了该信号的回调,看看会返回什么错误
```text
    Broken pipe
```
+ 读一个已经收到了rst回复的套接字会以Connection reset by peer退出，写一个已经收到了rst回复的套接字会收到SIGPIPE信号,默认退出。如果忽略了该信号,则write系统调用会返回Broken pipe错误

## 链接过程中主机崩溃
+ 客户端不断尝试发起tcp报文，最后因Operation timeout返回[在实际应用中，这个时间一般是70s左右，对于大部分场景来说这太长了，而改变源码选项重新编译也不太合适。所以一般的应用层协议都会定义自己的超时时间和握手规则]

## 链接过程中主机崩溃后恢复
+ 客户端不断尝试发起tcp报文，当服务端重启后，原来内核中维护的套接字状态(tcp链接)被重置，客户端成功发起tcp请求后会得到rst分节的回复。此时客户端处于read系统调用，将会返回Connection by peer
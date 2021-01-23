//
// Created by bytedance on 2020/8/28.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"
#include <string.h>
#include "../helper/helper.h"
#include "../types/types.h"
#include <unistd.h>


void start_server(int port,int back_log) {
    /**
     * PF_INET|AF_INET 指定网际层为ipv4协议
     * SOCK_STREAM TCP流式套接字
     */
    int socket_fd = socket(PF_INET,SOCK_STREAM,0);//调用socket函数,返回一个待绑定和监听的套接字描述符
    //fd为负数,说明出现了错误,此时错误会记录在全局变量中
    if (socket_fd < 0) {
        ERR_CHECK(get_errno())
    }

    struct sockaddr_in server_addr; //服务端套接字结构
    socklen_t server_addr_len = sizeof(server_addr); //获取sockaddr_in的长度
    bzero(&server_addr,server_addr_len); //c语言没有gc,创建变量时要么需要初始化,要么声明成全局或者静态变量,或者使用bzero等宏清空空间(特别是数组或者结构体类型的变量)

    server_addr.sin_family = AF_INET; //ipv4网际层协议
    server_addr.sin_port = htons(port); //int型的端口号转换成4个字节的二进制表示
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //绑定主机上的所有网卡地址(内外网)

    int result;
    result = bind(socket_fd, (SA *) &server_addr, server_addr_len); //result小于0说明bind错误
    if (result < 0) {
        ERR_CHECK(get_errno())
    }

    //No, that's not necessary -- as long as you're accepting connections as soon as they come in, the length of your listen backlog is irrelevant. You can have as many active connections as you need; the listen backlog only affects connections which haven't been fully established
    result = listen(socket_fd,back_log); //指定backlog=200(等待三次握手的连接数(所有未处于established的连接),超出200时服务端不响应客户端的任何握手请求(也不会回复RST),将会触发超时重传)
    if (result < 0) {
        ERR_CHECK(get_errno())
    }
    /**
     * 1) 全缓冲
        当填满标准I/O缓存后才进行实际I/O操作。全缓冲的典型代表是对磁盘文件的读写。

       2) 行缓冲
        当在输入和输出中遇到换行符时，执行真正的I/O操作。这时，我们输入的字符先存放在缓冲区，等按下回车键换行时才进行实际的I/O操作。典型代表是标准输入(stdin)和标准输出(stdout)。

       3) 不带缓冲
        也就是不进行缓冲，标准出错情况stderr是典型代表，这使得出错信息可以直接尽快地显示出来。
     */
     printf("listening...\n"); //属于行缓冲,遇到换行符才输出缓冲数据到标准输出流
     //也可以使用 fflush(stdout); 马上刷新缓冲数据

    int connect_fd;
    while (1) {
        connect_fd = accept(socket_fd, (SA *) &server_addr, &server_addr_len);
        if (connect_fd < 0 ) {
            ERR_CHECK(get_errno())
        }
        printf("accept one connection\n");
        char buffer[] = "hello world!";
        result = writen(connect_fd,buffer,strlen(buffer));
        if (result == 0) {
            close(connect_fd);
            printf("%s","remote client close the connection");
        } else if (result < 0) {
            ERR_CHECK(get_errno())
        }
        printf("success in send %d bytes,content is %s\n",result,buffer);
        bzero(buffer,sizeof(buffer));
        //close(connect_fd);
        printf("connection close\n");
    }
}
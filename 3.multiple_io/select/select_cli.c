//
// Created by bytedance on 2020/12/11.
//

#include <sys/select.h>
#include "select_cli.h"
#include "../../types/types.h"
#include "../../helper/helper.h"
#include <string.h>
#include <stdio.h>
#define MAX_SIZE 65535

 //使用select多路复用函数构建的客户端
//@param FILE *fp 已经打开的文件
//@param int socket_fd 已经建立了连接的套接字描述符
static void handle(FILE *fp,int socket_fd) {
    fd_set rset; //文件描述符集合,其实是一个bitmap
    boolean is_eof = FALSE;//从文件流中是否读取到了EOF
    int file_fd = fileno(fp); //把已经打开的文件流转化为文件描述符。0是标准输入流 1是标准输出流 2是标准错误流
    char buffer[MAX_SIZE];
    FD_ZERO(&rset); //清空bitmap
    int max_fd;

    //循环
    while (TRUE) {
        //如果没有读取到文件流中的EOF,则设置对文件描述符的监听
        if (FALSE == is_eof) {
            FD_SET(file_fd,&rset); //添加文件描述符到待观察的bitmap中
            FD_SET(socket_fd,&rset); //添加套接字描述符到待观察的bitmap中
            max_fd = Max(file_fd,socket_fd) + 1; //取得描述符的最大值,减少检测套接字是否ready中检测的O(n)中的n
        } else {
            FD_SET(socket_fd,&rset); //添加套接字描述符到待观察的bitmap中
            max_fd = socket_fd; //取得描述符的最大值,减少检测套接字是否ready中检测的O(n)中的n
        }
        //第一个参数 第一个参数是待观察的bitmap中最大的描述符+1,用于减少循环次数
        //第二个参数 返回可读事件的套接字描述符集合
            //-- 1.套接字接收缓存区中的可读数据段大于socket_option设置的最低水位
            //-- 2.接受到对端发出的FIN包
            //-- 3.连接的过程中发生了错误,比如接收到了RST分节
            //-- 4.被动套接字(listen)的backlog中存在已完成三次握手的连接。
        //第三个参数 返回可写事件的套接字描述符集合
            //-- 1.套接字发送缓存区中的可写数据段大于socket_option设置的最低水位
            //-- 2.连接的过程中发生了错误[错误都是可写可读的]
            //-- 3.非阻塞的connect调用完成了三次握手
        //第四个参数 返回错误事件的套接字描述符集合
        //第五个参数 select阻塞的时间,如果设置为NULL,则阻塞直到有描述符准备好为止
        select(max_fd + 1,&rset,NULL,NULL,NULL);

        //如果套接字描述符准备好了
        if (FD_ISSET(socket_fd,&rset)) {
            int result = read_once(socket_fd,buffer,MAX_SIZE);
            //接受到了fin包,正常退出
            if (0 == result) {
                printf("quit normal");
                return;
            } else if (result < 0) {
                ERR_PRINT("err")
            }
            //往标准输出流中写入数据
            writen(fileno(stdout), buffer, result);
            bzero(buffer,result);
            FD_CLR(socket_fd,&rset);
        }

        //如果文件描述符准备好了
        if (FD_ISSET(file_fd,&rset)) {
            int result = read_once(file_fd,buffer,MAX_SIZE);
            //读取到了EOF
            if (0 == result) {
                is_eof = TRUE;
                continue;
            } else if (result < 0) {
                ERR_PRINT("err")
            }
            //往对端套接字中写入数据
            writen(socket_fd, buffer, result);
            bzero(buffer,result);
            FD_CLR(file_fd,&rset);
        }
    }
}

void select_client() {
    int socket_fd = Connect("localhost",10000);
    if (socket_fd < 0) {
        exit(-1);
    }
    handle(stdin,socket_fd);
}
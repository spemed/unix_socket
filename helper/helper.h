//
// Created by bytedance on 2020/8/28.
//

#ifndef UNIX_SOCKET_HELPER_H
#define UNIX_SOCKET_HELPER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../types/types.h"


//检测check_error的result,检测失败直接退出进程
#define ERR_CHECK(x)        \
    if(x>0)                 \
    {                       \
        exit(1);            \
    }
//打印错误值并退出进程
#define ERR_PRINT(msg)          \
    {                           \
        perror(msg);            \
        exit(-1);               \
    }                           \

/**
 *
 * @return 1 检测到错误
 * @return 0 没有检测到错误
 * 这里的错误使用errno.h中的宏errno,是个全局变量,线程不安全
 */
int get_errno();
int writen(int fd,const char* buffer,int buffer_size);
int readn(int fd,char* buffer,int buffer_size);
int read_once(int fd,char* buffer,int buffer_size);
int Connect(const char *ip,unsigned int port) ;
int Server(unsigned int port) ;
int Max(int a,int b);
sigfunc *signal_setter(int,sigfunc*);
#endif //UNIX_SOCKET_HELPER_H

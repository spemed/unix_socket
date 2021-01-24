//
// Created by bytedance on 2021/1/24.
//

#include <unistd.h>
#include "so_linger.h"

static char buffer[90000] = {'\0'};
static void write_90000_bytes(int socket_fd) {
    for (int i=0;i<sizeof(buffer)-1;i++) {
        buffer[i] = 'a';
    }
    int result = write(socket_fd,buffer,sizeof(buffer));
    if (-1 == result) {
        ERR_PRINT("failed to write")
    }
}

void server_handle(int port) {
    char buf[90000];
   int fd = Server(port);
   if (-1 == fd) {
       return;
   }
   //睡眠30s后sleep
   sleep(30);
   int result = readn(fd,buf,sizeof(buf));
   if (-1 == result) {
       ERR_PRINT("get rst packet")
   } else if (0 == result) {
       //接受到了90000个字节
       printf("get all data");
   } else {
       //对端发送了fin包导致接受的字节数小于目标
       printf("remote client close the connection");
   }
}

void client_handle(int port) {
    //三路握手成功客户端返回已连接套接字
    int connect_fd = Connect("127.0.0.1",port);
    if (-1 == connect_fd) {
        return;
    }
    //写入9万字节
    write_90000_bytes(connect_fd);
#ifdef BLOCK_SOCKET_CLOSE_NORMAL
    block_socket_close_normal();
#endif
#ifdef BLOCK_SOCKET_CLOSE_SO_LINGER_ON
    block_socket_close_so_linger_on(connect_fd);
#endif
#ifdef BLOCK_SOCKET_CLOSE_SO_LINGER_ON_AND_SET_SECONDS
    block_socket_close_so_linger_on_and_set_seconds(connect_fd,10);
#endif
    return;
}

void block_socket_close_normal(int socket_fd) {
    printf("close normal");
    close(socket_fd);
}

void block_socket_close_so_linger_on(int socket_fd) {
    block_socket_close_so_linger_on_and_set_seconds(socket_fd,0);
}

void block_socket_close_so_linger_on_and_set_seconds(int socket_fd,int seconds) {
    struct linger linger = {
        .l_onoff = 1,
        .l_linger = seconds
    };
    int result = setsockopt(socket_fd,SOL_SOCKET,SO_LINGER,&linger,sizeof(linger));
    if (-1 == result) {
        ERR_PRINT("failed to set socket option")
    }
}
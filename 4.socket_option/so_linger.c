//
// Created by bytedance on 2021/1/24.
//

#include <unistd.h>
#include <arpa/inet.h>
#include "so_linger.h"
#include "show_all_options.h"
#include "string.h"

static char buffer[90000] = {'\0'};
static void write_90000_bytes(int socket_fd) {
    for (int i=0;i<sizeof(buffer)-1;i++) {
        buffer[i] = 'a';
    }
    int result = writen(socket_fd,buffer,sizeof(buffer));
    if (-1 == result) {
        ERR_PRINT("failed to write")
    }
}

void server_handle(int port) {
   int fd = Server(port);
   if (-1 == fd) {
       return;
   }
   struct sockaddr_in server_addr;
   socklen_t server_addr_len = sizeof(server_addr);
   int connect_fd = accept(fd,(SA*)&server_addr,&server_addr_len);
   if (-1 == connect_fd) {
       ERR_PRINT("failed to accept")
   }
   //睡眠30s后sleep
   sleep(10);
   int result = readn(connect_fd,buffer,sizeof(buffer));
   printf("read bytes=%d\n",result);
   if (-1 == result) {
       ERR_PRINT("get rst packet")
   } else if (sizeof(buffer) == result) {
       //接受到了90000个字节
       printf("get all data");
   } else {
       //对端发送了fin包导致接受的字节数小于目标
       printf("remote client close the connection");
   }
}

#define BLOCK_SOCKET_CLOSE_SO_LINGER_ON
void client_handle(int port) {

    //三路握手成功客户端返回已连接套接字
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if (socket_fd < 0) {
        ERR_PRINT("failed to exec socket()")
    }

#ifdef BLOCK_SOCKET_CLOSE_SO_LINGER_ON
    block_socket_close_so_linger_on(socket_fd);
#endif

#ifdef BLOCK_SOCKET_CLOSE_SO_LINGER_ON_AND_SET_SECONDS
    block_socket_close_so_linger_on_and_set_seconds(connect_fd,10);
#endif

    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    bzero(&cli_addr,cli_addr_len);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);
    int result = inet_aton("127.0.0.1",&cli_addr.sin_addr);
    if (result < 0) {
        ERR_PRINT("invalid ip address");
    }

    result = connect(socket_fd,(SA *)&cli_addr,cli_addr_len);
    if (result < 0) {
        ERR_PRINT("failed in connect")
    }

    //写入9万字节
    write_90000_bytes(socket_fd);
    close_normal(socket_fd);
}

void close_normal(int socket_fd) {
    printf("close normal");
    close(socket_fd);
}

void block_socket_close_so_linger_on(int socket_fd) {
    block_socket_close_so_linger_on_and_set_seconds(socket_fd,0);
    printf("close with so_linger on\n");
//    close(socket_fd);
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
//    print_socket_options();
}
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
    int result = write(socket_fd,buffer,sizeof(buffer));
    if (-1 == result) {
        ERR_PRINT("failed to write")
    }
}

void server_handle(int port) {
    int fd = Server(port);
    if (-1 == fd) {
        return;
    }
    while (TRUE) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int connect_fd = accept(fd,(SA*)&client_addr,&client_addr_len);
        if (-1 == connect_fd) {
            ERR_PRINT("failed to accept")
        }
        //睡眠30s后sleep
        sleep(10);
        int result;
        do {
            result = readn(connect_fd,buffer,sizeof(buffer));
            printf("read bytes=%d\n",result);
            //LEVEL = SOL_SOCKET,SO_ERROR可被读取不可以被设置
            //read的时候如果发现出现了错误,则由SO_ERROR的值写到全局变量errno中,并置位为0
            if (-1 == result) {
                perror("get rst packet\n");
                continue;
            } else if (0 == result) {
                //对端发送了fin包导致接受的字节数小于目标
                printf("remote client close the connection\n");
                close(connect_fd);
            } else {
                //接受到了90000个字节
                printf("get all data\n");
            }
        } while (!(result == -1 || result == 0));
    }
}

#define BLOCK_SOCKET_CLOSE_SO_LINGER_ON_AND_SET_SECONDS
void client_handle(int port) {

    //三路握手成功客户端返回已连接套接字
    int connect_fd = Connect("127.0.0.1",port);
    if (connect_fd < 0) {
        ERR_PRINT("failed to exec Connect()")
    }

#ifdef BLOCK_SOCKET_CLOSE_SO_LINGER_ON
    block_socket_close_so_linger_on(connect_fd);
#endif

#ifdef BLOCK_SOCKET_CLOSE_SO_LINGER_ON_AND_SET_SECONDS
    block_socket_close_so_linger_on_and_set_seconds(connect_fd,10);
#endif
    val val;
    socklen_t val_len = sizeof(val);
    getsockopt(connect_fd,SOL_SOCKET,SO_LINGER,&val,&val_len);
    printf("l_onoff=%d,l_linger=%d\n",val.linger_val.l_onoff,val.linger_val.l_linger);
    //写入9万字节
    write_90000_bytes(connect_fd);
    close(connect_fd);
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
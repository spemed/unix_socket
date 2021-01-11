//
// Created by bytedance on 2021/1/11.
//

#include "poll_server.h"
#include <limits.h>
#include <poll.h>
#include <zconf.h>
#include <string.h>
#include <arpa/inet.h>
#include "../../types/types.h"
#define INFTIM -1
#define BUFFSIZE 65535

typedef struct pollfd pollfd;

//todo 有bug,多个客户端断开连接的时候,仍然存在部分close_wait的连接
/**
 *   new connection ip=127.0.0.1,port=54513
    new connection ip=127.0.0.1,port=54522
    new connection ip=127.0.0.1,port=54536
    new connection ip=127.0.0.1,port=54545
    new connection ip=127.0.0.1,port=54554
    connection close from client
 * @param port
 */
_Noreturn void poll_server(unsigned int port) {
    //OPEN_MAX为进程允许打开的最大描述符数目
    pollfd client_arr[OPEN_MAX];
    int maxi = 0;
    //初始化所有pollfd
    for (int i=0;i<OPEN_MAX;i++) {
        client_arr[i].fd = -1;
    }
    int listen_fd = Server(port);
    if (listen_fd < 0) {
        ERR_PRINT("failed to create listen_fd");
    }

    client_arr[0].fd = listen_fd;
    //listen套接字上存在完成三次握手的连接既属于普通数据又属于优先级数据,一般认为是普通数据
    client_arr[0].events = POLLRDNORM;
    int nready;

    char buffer[BUFFSIZE];
    bzero(buffer,BUFFSIZE);

    //死循环
    while (TRUE) {
        //poll的第三参数＜0,永久等待。=0,不阻塞,> 0 阻塞该值的毫秒数
        nready = poll(client_arr,maxi + 1,INFTIM);
        if (client_arr[0].revents & POLLRDNORM) {
            //说明监听套接字上存在可用连接
            struct sockaddr_in client_socket;
            socklen_t client_socket_len = sizeof(client_socket);
            int connect_fd = accept(client_arr[0].fd,(SA*)&client_socket,&client_socket_len);
            if (connect_fd < 0) {
                printf("failed to get connect_fd");
                if (--nready <= 0) {
                    continue;
                }
                goto handle;
                //todo 跳转到
            }
            printf("new connection ip=%s,port=%d\n",inet_ntoa(client_socket.sin_addr),ntohs(client_socket.sin_port));
            int i = 1;
            for (;i < OPEN_MAX; ++i) {
                if (client_arr[i].fd == -1) {
                    client_arr[i].fd = connect_fd;
                    client_arr[i].events = POLLRDNORM;
                    break;
                }
            }
            if (i == OPEN_MAX) {
                printf("too much connection\n");
                close(connect_fd);
                if (--nready <= 0) {
                    continue;
                }
                goto handle;
            } else if (i > maxi){
                 maxi = i;
            }
            if (--nready <= 0) {
                continue;
            }
        }

        handle: for (int j = maxi; j < OPEN_MAX; ++j) {
            if (client_arr[j].fd == -1)  continue;
            if (client_arr[j].revents & (POLLRDNORM|POLLERR)) {
                int read_bytes = read_once(client_arr[j].fd,buffer,BUFFSIZE);
                if(read_bytes < 0) {
                    //接收到了来自客户端的tcp分节
                    if (errno == ECONNRESET) {
                        printf("connection reset by peer");
                        close(client_arr[j].fd);
                        client_arr[j].fd = -1;
                    } else {
                        perror("another error");
                    }
                } else if (0 == read_bytes) {
                    //收到fin包
                    close(client_arr[j].fd);
                    client_arr[j].fd = -1;
                    printf("connection close from client\n");
                } else {
                    writen(client_arr[j].fd,buffer,read_bytes);
                }
            }
            if (--nready <= 0) break;
        }
    }
}
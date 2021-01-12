//
// Created by bytedance on 2021/1/11.
//

#include "poll_cli.h"
#include "../../helper/helper.h"
#include <poll.h>
#include <string.h>
#include <unistd.h>

//todo 检查一下,为什么无法发送消息
void handle(FILE* file,int socket_fd) {
    int file_fd = fileno(file);
    pollfd client_arr[2];
    client_arr[0].fd = file_fd;
    client_arr[0].events = POLLRDNORM;
    client_arr[1].fd = socket_fd;
    client_arr[1].events = POLLRDNORM;

    char buffer[BUFFSIZE];
    bzero(buffer,sizeof(buffer));

    //死循环
    while (TRUE) {
        poll(client_arr,3,INFTIM);
        //如果套接字返回
        if (client_arr[0].revents & (POLLRDNORM|POLLERR)) {
            //如果已经读到了文件终止符,则关闭写一侧
            int read_bytes = read_once(client_arr[0].fd,buffer,BUFFSIZE);
            //发生了错误
            if (read_bytes < 0) {
                close(client_arr[0].fd);
                ERR_PRINT("ERR WHEN CONNECTION");
            } else if (0 == read_bytes) {
                close(client_arr[0].fd);
                printf("exit normally\n");
                exit(0);
            } else {
                //写入标准输出流
                writen(fileno(stdout),buffer,read_bytes);
            }
        }
        //如果输入流返回
        if (client_arr[1].revents & (POLLRDNORM|POLLERR)) {
            int read_bytes = read_once(client_arr[1].fd,buffer,BUFFSIZE);
            //发生了错误
            if (read_bytes < 0) {
                ERR_PRINT("ERR WHEN READ FROM STDIN")
            } else if (0 == read_bytes) {
                //读取到了eof
                client_arr[1].fd = -1;
                shutdown(socket_fd,SHUT_WR);
            } else {
                //写入标准输出流
                writen(socket_fd,buffer,read_bytes);
            }
        }
    }
}

void poll_cli(const char *ip,unsigned int port) {
    int connect_fd = Connect(ip,port);
    if (connect_fd < 0) {
        ERR_PRINT("failed to create connect_fd")
    }
    handle(stdin,connect_fd);
}
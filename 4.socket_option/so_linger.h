//
// Created by bytedance on 2021/1/24.
//

#ifndef SELECT_CLIENT_SO_LINGER_H
#define SELECT_CLIENT_SO_LINGER_H
#include "../helper/helper.h"

void client_handle(int port);
void server_handle(int port);
//阻塞型io
void close_normal(int socket_fd); //正常关闭
void block_socket_close_so_linger_on(int socket_fd); //启动so_linger且秒数设置为0
void block_socket_close_so_linger_on_and_set_seconds(int socket_fd,int seconds); //启动so_linger且设置秒数
//非阻塞io
void non_block_socket_close_so_linger_on(); //启动so_linger且秒数设置为0
void non_block_socket_close_so_linger_on_and_set_seconds(int seconds); //启动so_linger且设置秒数
#endif //SELECT_CLIENT_SO_LINGER_H

//
// Created by bytedance on 2020/8/28.
// 创建一个用于聊天的客户端
//

#ifndef UNIX_SOCKET_CLIENT_H
#define UNIX_SOCKET_CLIENT_H

#include "../helper/helper.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../types/types.h"
#include <unistd.h>

void start_client(const char*ip_str,int port);

#endif //UNIX_SOCKET_CLIENT_H

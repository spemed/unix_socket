//
// Created by bytedance on 2020/10/19.
//

#ifndef UNIX_SOCKET_SERVER_CONNECT_H
#define UNIX_SOCKET_SERVER_CONNECT_H

#include <sys/socket.h>
#include "../helper/helper.h"
#include "../types/types.h"
#include <memory.h>
#include <arpa/inet.h>
#include <zconf.h>

void time_client(char *ip_str,int port);
#endif //UNIX_SOCKET_SERVER_CONNECT_H

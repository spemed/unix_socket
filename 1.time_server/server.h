//
// Created by bytedance on 2020/10/19.
//

#ifndef UNIX_SOCKET_SERVER_SERVER_H
#define UNIX_SOCKET_SERVER_SERVER_H
#endif //UNIX_SOCKET_SERVER_SERVER_H

#include <stdio.h>
#include <sys/socket.h>
#include "../helper/helper.h"
#include "../types//types.h"
#include <netinet/in.h>
#include <string.h>
#include <zconf.h>
#include <sys/time.h>
#include <arpa/inet.h>
_Noreturn void time_server(int port);
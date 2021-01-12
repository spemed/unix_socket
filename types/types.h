//
// Created by bytedance on 2020/8/28.
//

#ifndef UNIX_SOCKET_TYPES_H
#define UNIX_SOCKET_TYPES_H
typedef struct sockaddr SA;
typedef void sigfunc(int) ;
typedef int boolean;
typedef struct pollfd pollfd;
#define TRUE 1
#define FALSE 0
#define INFTIM -1
#define BUFFSIZE 65535
#endif //UNIX_SOCKET_TYPES_H

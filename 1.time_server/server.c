//
// Created by bytedance on 2020/10/19.
//


#include "server.h"

void  get_time(char *buffer,int size) {
    time_t now ;
    struct tm *tm_now ;
    time(&now) ;
    tm_now = localtime(&now) ;
    snprintf(buffer,size,"%d-%d-%d %d:%d:%d",tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
}

_Noreturn void time_server(int port) {
    printf("program start...\n");
    int socket_fd = socket(AF_INET,SOCK_STREAM,0); //ipv4,流式套接字
    if(socket_fd < 0) {
        ERR_PRINT("failed to execute socket()...")
    }

    struct sockaddr_in servaddr,clientaddr;
    socklen_t servaddr_len = sizeof(servaddr),clientaddr_len = sizeof(clientaddr);
    bzero(&servaddr,servaddr_len); //结构体置空
    bzero(&clientaddr,clientaddr_len); //结构体置空

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //监听内外网
    servaddr.sin_port = htons(port); //监听端口

    //指定监听的端口以及宿主机(内外网)
    int result = bind(socket_fd,(SA*)&servaddr,sizeof(servaddr));
    if ( result < 0 ) {
        ERR_PRINT("failed to execute bind()...")
    }

    //开始监听,backlog指定为100
    result = listen(socket_fd,100);
    if (result < 0 ) {
        ERR_PRINT("failed to execute listen()...")
    }

    //循环服务器,只能按序处理请求
    while (1) {
        //返回已连接套接字
        int connect_fd = accept(socket_fd, (SA*)&clientaddr,&clientaddr_len);
        if (connect_fd < 0) {
            ERR_PRINT("failed to execute accept()...")
        }
        getsockname(connect_fd,(SA*)&clientaddr,&clientaddr_len);
        printf("client ip=%s\n",inet_ntoa(clientaddr.sin_addr));
        printf("client port=%d\n",ntohs(clientaddr.sin_port));

        char buffer[50] = {'\0'};
        //获取当前unix时间戳
        get_time(buffer,40);
        //把结果转字符串写入

        int nwrite = writen(connect_fd,buffer,(int )strlen(buffer));
        if (nwrite < 0) {
            ERR_PRINT("failed to execute write()...")
        }
        //关闭已连接套接字
        close(connect_fd);
        bzero(buffer,sizeof(buffer));
    }
}
//
// Created by bytedance on 2020/10/19.
//

#include "client.h"


void time_client(char *ip_str,int port) {
    printf("client program start...\n");
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd < 0){
        ERR_PRINT("failed to socket()")
    }
    struct sockaddr_in serv_addr;
    socklen_t serv_addr_len = sizeof(serv_addr);
    bzero(&serv_addr,serv_addr_len);

    if( !inet_aton(ip_str, &serv_addr.sin_addr)) {
        ERR_PRINT("invalid ip str")
    }

    serv_addr.sin_port = htons(port);
    serv_addr.sin_family = AF_INET;

    int result = connect(socket_fd,(SA*)&serv_addr,serv_addr_len);
    if (result < 0) {
        ERR_PRINT("failed to connect()");
    }

    //连接成功后开始获取时间
    char buffer[50] = {'\0'};
    int nread = read_once(socket_fd,buffer,50);
    if (nread < 0) {
        ERR_PRINT("failed in read");
    }
    printf("nread=%d\n",nread);
    close(socket_fd);

    printf("current time=%s\n",buffer);
    printf("program client stop...\n");

}
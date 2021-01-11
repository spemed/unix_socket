//
// Created by bytedance on 2020/8/28.
//

#include "client.h"


void start_client(const char* ip_str,int port) {
    int fd = socket(PF_INET,SOCK_STREAM,0);
    if (fd < 0) {
        ERR_CHECK(get_errno())
    }
    struct sockaddr_in server_addr; //服务端套接字结构
    server_addr.sin_family = AF_INET; //ipv4网际层协议
    server_addr.sin_port = htons(port); //指定服务端端口号
    server_addr.sin_addr.s_addr = inet_addr(ip_str); //指定服务端的ip

    size_t server_addr_len = sizeof(server_addr);
    int result = connect(fd,(SA *)&server_addr,server_addr_len);
    if(result < 0) {
        ERR_CHECK(get_errno())
    }

    //成功完成三次握手后
    printf("success in connection...\n"); //加上换行符用于刷新行缓冲
    char buffer[255] = {'\0'};
    result = readn(fd,buffer,255);
    if (result < 0) {
        ERR_CHECK(result)
    } else if (0 == result) {
        close(fd);
        printf("remote server close the connection\n");
    }
    printf("recv buffer is %s\n",buffer);
    close(fd);
    printf("connection close...\n");
}

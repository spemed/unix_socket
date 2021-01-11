//
// Created by bytedance on 2020/12/17.
//

#include <ntsid.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "select_ser.h"
#include "../../helper/helper.h"
static void handle(int port) {

}

//todo 存在bug 每次只能管理一个连接
#define BUFFER_MAX 65535
const int fd_set_size = FD_SETSIZE - 1;

/**
 * 特点：使用bitmap作为连接状态的维护,占用空间小
 * 优点：posix操作系统对select支持比较好,移植性强,甚至提供了pselect这种更加精细的阻塞时间控制以及信号处理的函数
 * 缺点：1.使用上较为麻烦,因为bitmap是内核维护,所以需要传递第一个参数maxfd用于缩短内核的遍历时间
 *      [todo 以前是存在限制的,限制是否存在限制待验证]
 *      2.同样受到bitmap的限制,单个进程能够监视的文件描述符的数量存在最大限制,在 Linux 上一般为 1024，可以通过修改宏定义甚至重新编译内核的方式提升这一限制，但是这样也会造成效率的降低
 */
void select_server() {

    int listen_socket_fd = Server(10000);
    //创建被动套接字失败
    if (listen_socket_fd < 0) {
        ERR_PRINT("failed to create listen socket")
    }

    //FD_SETSIZE为select可以监听描述符的最大限值
    int client_arr[fd_set_size];
    //初始化client_arr的每一项为-1
    for(int i=0;i<fd_set_size;i++){
        client_arr[i] = -1;
    }


    int maxfd,maxi = -1; //maxfd用于存放套接字的最大值,maxi用于存放已连接客户端的套接字
    fd_set rset,allset;
    FD_ZERO(&allset); //allset内存清空垃圾值
    maxfd = listen_socket_fd;//设置listen_socket_fd为maxfd
    int nready;
    char buffer[BUFFER_MAX];
    FD_SET(listen_socket_fd,&allset);

    for(;;) {
        rset = allset;
        //返回本次活跃的套接字数目
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);

        //判断是否是被动套接字返回了
        if (FD_ISSET(listen_socket_fd,&rset)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_addr_len;
            //accpet返回主动套接字
            int client_fd = accept(listen_socket_fd,(SA*)&cli_addr,&cli_addr_len);
            printf("new connection ip=%s,port=%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));
            if (client_fd < 0) {
                perror("failed to accept\n");
                goto event;
            }

            //从client_arr中找到任意一个空位填写进去
            int i = 0;
            for (; i < fd_set_size; ++i) {
                if (client_arr[i] == -1 ) {
                    client_arr[i] = client_fd;
                    break;
                }
            }
            //如果已经达到了select观察链接的最大数目,
            if (i == fd_set_size) {
                close(client_fd);
                perror("quit,too many connections");
                goto event;
            } else {
                FD_SET(client_fd,&allset);
                //更新maxfd
                if(client_fd > maxfd) {
                    maxfd = client_fd;
                }
                //更新maxi
                if (i > maxi) {
                    maxi = i;
                }
            }
//            //如果已经处理完本次返回的所有连接了,则退出等待下一轮循环
//            if (--nready <= 0) {
//                continue;
//            }
        }

        //处理主动套接字
        event: for (int j = 0; j <= maxi; ++j) {
            if (client_arr[j] == -1) {
                continue;
            }
            if (!FD_ISSET(client_arr[j],&rset)) {
                continue;
            }
            int n = read_once(client_arr[j],buffer,BUFFER_MAX);
            //读取到eof字符,对端已经发送fin包
            if (n == 0) {
                //关闭连接
                close(client_arr[j]);
                client_arr[j] = -1; //关闭活跃连接
                FD_CLR(client_arr[j],&allset);//select对该套接字不在感兴趣
                printf("connection:%d terminate\n",j);
            } else if(n < 0) {
                perror("error in connection"); //某个主动套接字发生了错误
            } else {
                writen(client_arr[j],buffer,n);
            }
            //如果已经处理完本次返回的所有连接了,则退出等待下一轮循环
            if (--nready <= 0) {
                break;
            }
        }
    }


}
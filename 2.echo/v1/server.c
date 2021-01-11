//
// Created by bytedance on 2020/10/29.
//

#include "server.h"
#include "../../common/common.h"
#include "arpa/inet.h"

#define MAX_BUFFER_LEN 10000

static void _server_echo(int connect_fd) {
    //此处没有在每次read的时候都堆buffer做bzero
    //因为在回写的时候仅写入从read中读到的n个字节
    char buffer[MAX_BUFFER_LEN] = {'\0'};
    int n_read;
    while (1) {
        //读取到buffer中
        n_read = read(connect_fd,buffer,MAX_BUFFER_LEN);
        //如果是被信号打断的错误,重新读取
        if (n_read < 0 && errno == EINTR) {
            continue;
        } else if (n_read < 0 && errno != EINTR) {
            //如果不是被信号打断的错误,直接退出
            break;
        } else if (0 == n_read) {
            //说明已经接受到对端发送的fin分节,服务端连接处于半关闭状态
            //直接退出循环
            break;
        }
        //原样写给客户端
        int result = writen(connect_fd,buffer,n_read);
        if (result < 0) {
            printf("buffer=%s,len=%d",buffer,n_read);
            perror("failed to writen to client");
        }
    }
}

/**
   回声程序服务端实现
   1.每个请求都触发一次fork进程,在子进程中处理业务信息[并发服务器]
   2.1 父进程阻塞于accept调用等待可用连接
   2.2 子进程阻塞于read调用，直到取得客户端发送的字符串[tcp分节]
   2.3 调用write,回送该字符串
   2.4 继续阻塞于read调用,等待下一次请求
*/
/**
 *  不足
 *     子进程正常退出后,并不会直接进入终止状态。而是会处于Z(僵死状态)等待父进程收集子进程的退出码等统计信息。
 *     如果父进程没有主动去收集这部分信息(waitpid)。或者为子进程设置对忽略退出码信号[记不清楚了,要回去看看]
 *     则子进程会一直处于僵死状态。虽然不在接受调度，但是会占用内核存储[pcb仍保留有进程记录]，浪费系统资源
 *
 *
 *  客户端进程退出前
        PID  PPID TTY      STAT ARGS          WCHAN
        92115 90214 ttys002  S+   ./server_echo -
        92248 92115 ttys002  S+   ./server_echo -
        92247 90464 ttys005  S+   ./cli_echo    -
    客户端进程退出后
        PID  PPID TTY      STAT ARGS          WCHAN
        92115 90214 ttys002  S+   ./server_echo -
        92248 92115 ttys002  Z+   (server_echo) -

    可以看到,子进程[92248]处于僵死状态
 */
_Noreturn void server_echo() {
    int listen_fd = Server(SERVER_PORT);
    if (listen_fd < 0) {
        ERR_PRINT("failed to listen")
    }
    int pid;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    bzero(&client_addr,client_addr_len);
    sleep(10);
    while (1) {
        int connect_fd = accept(listen_fd,(SA*)&client_addr,&client_addr_len);
        if (connect_fd < 0 ) {
            perror("failed to accept");
            continue;
        }
        printf("get new connection,client.ip=%s,client.port=%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        pid = fork();
        //子进程
        if (0 == pid) {
            printf("son progress start...\n");
            //关闭被动套接字[因为fork导致引用计数加一]
            close(listen_fd);
            //回声工作开始进行
            _server_echo(connect_fd);
            printf("son progress stop...\n");
            exit(0);
        } else {
            //父进程关闭已连接套接字[因为fork导致引用计数加一]
            close(connect_fd);
        }
    }
}
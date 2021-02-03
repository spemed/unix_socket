//
// Created by bytedance on 2020/8/28.
//

#include <zconf.h>
#include "helper.h"
#include <arpa/inet.h>
#include <signal.h>
#include "string.h"

int get_errno(const char *message) {
    if(errno > 0) {
        perror(message);
    }
    return errno;
}

/**
 * 自己封装的writen函数,相比原始write函数增加了下列功能
 * write函数在把应用进程内存缓存区中的数据写入到操作系统内核中的输出缓存区后返回
 * 返回本次写入的字节数
 * 大于0，且等于要求写入的字节数，属于正常情况
 * 大于0，且小于要求写入的字节数，说明输出缓存区已满,writen函数的目的就是为了使得出现情况2等待直到数据完全写入缓冲区才返回
 * 小于0, 返回错误。但是需要注意慢系统调用(可能永久阻塞的系统调用)被软件中断(信号)打断时,errno=EINTR。当负责处理软件中断的例程(注册信号处理的回调函数)执行完毕时,进程会从慢系统调用处恢复执行,所以当发现errno=EINTR时,不需要退出程序,应该重新调用慢系统调用
 * @param fd
 * @param buffer
 * @param buffer_size
 * @return
 */
int writen(int fd,const char* buffer,int buffer_size) {
    int n_left = buffer_size; //还剩下未读的字节数
    char *ptr = buffer; //执行char数组的指针
    int n_write; //已经读取的字节数
    while (n_left > 0) {
        n_write = write(fd,ptr,buffer_size); //试图往fd中写入buffer_size的数据
        //写入出错,但可能是被信号打断的慢系统调用,需要加多个判断
        if (n_write < 0) {
            //被软件中断(信号)打断的慢系统调用,中断例程(对应信号注册的回调)会从此处返回,出现errno=EINTR时需要重新执行慢系统调用
            if (errno == EINTR) {
                n_write = 0;
            } else {
                return -1; //非打断错误直接返回-1
            }
        }
        n_left -= n_write; //减去已经写入的字节
        ptr += n_write; //指针前移
    }
    return buffer_size;
}

/**
 * 自己封装的readn函数,相比原始read函数增加了下列功能
 * 1.慢系统调用被打断时的恢复
 * 2.错误捕捉
 * 3.读取到目标字节之前不返回
 * 返回本次读取的字节数
 * 大于0，且等于要求写入的字节数，属于正常情况
 * 大于0，且小于要求写入的字节数，说明输出缓存区已满,read函数的目的就是为了使得出现情况2等待直到数据完全读取到应用层缓冲区才返回
 * 等于0，对端调用close函数使得tcp连接处于半关闭状态,本端对对端发送的FIN包做出响应,此时调用read将从得到EOF
 * 小于0, 返回错误。但是需要注意慢系统调用(可能永久阻塞的系统调用)被软件中断(信号)打断时,errno=EINTR。当负责处理软件中断的例程(注册信号处理的回调函数)执行完毕时,进程会从慢系统调用处恢复执行,所以当发现errno=EINTR时,不需要退出程序,应该重新调用慢系统调用
 * @param fd
 * @param buffer
 * @param buffer_size
 * @return
 */
int readn(int fd,char* buffer,int buffer_size) {
    char *ptr = buffer;
    int n_left = buffer_size;
    int n_read;
    while (n_left > 0) {
        n_read = read(fd,buffer,buffer_size);
        //出现了错误
        if (n_read < 0) {
            //被软件中断(信号)打断的慢系统调用,中断例程(对应信号注册的回调)会从此处返回,出现errno=EINTR时需要重新执行慢系统调用
            if (errno == EINTR) {
                n_read = 0;
            } else {
                return -1;
            }
        }
        //返回0字节,说明tcp连接处于半关闭状态,本端tcp连接收到对端发出的FIN包,把当前套接字标识为不可读,每次读取时返回EOF,读取0个字节
        if (0 == n_read) {
            break;
        }
        //正常返回读到到的字节数
        n_left -= n_read;
        ptr += n_read;
    }
    return buffer_size - n_left;
}


int read_once(int fd,char* buffer,int buffer_size) {
    int result = read(fd,buffer,buffer_size);
    while (1) {
        if(result < 0 ) {
            if(errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        } else if (0 == result) {
            return 0;
        } else {
            return  result;
        }
    }
}

//自定义connect函数,传入ip字符串和端口完成connect。
//附带错误检测
//目前无法兼容ipv6,有待提高
int Connect(const char *ip,unsigned int port) {
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if (socket_fd < 0) {
        perror("failed to exec socket()");
        return -1;
    }
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    bzero(&cli_addr,cli_addr_len);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);
    int result = inet_aton(ip,&cli_addr.sin_addr);
    if (result < 0) {
        perror("invalid ip address");
        return -1;
    }

    result = connect(socket_fd,(SA *)&cli_addr,cli_addr_len);
    if (result < 0) {
        perror("failed in connect");
        return -1;
    }

    return socket_fd;
}

//自定义Server函数,传入ip字符串和端口完成listen。
//附带错误检测
//目前无法兼容ipv6,有待提高
#define BACKLOG_DEFAULT 100
int Server(unsigned int port) {
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if (socket_fd < 0) {
        perror("failed to exec socket()");
        return -1;
    }
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    bzero(&server_addr,server_addr_len);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int result = bind(socket_fd,(SA*)&server_addr,server_addr_len);
    if (result < 0) {
        perror("failed to bind");
        return -1;
    }

    result = listen(socket_fd,BACKLOG_DEFAULT);
    if (result < 0) {
        perror("failed in listen");
        return -1;
    }

    return socket_fd;
}

//设置信号监听函数的函数
// @param int signo 捕捉到的信号
// @param sigfunc *func 指向信号回调函数的函数指针
sigfunc *signal_setter(int signo,sigfunc* func) {
    struct sigaction act,oact;
    act.sa_handler = func; //接受到信号时需要执行的回调
    //todo 此处的阻塞递交,是指 1.进程在执行信号回调的过程中,内核阻止了把信号递交给进程的行为 还是 2.进程在执行回调的过程中,接受到被阻塞的信号也暂时不执行其回调[这个需要在多线程环境下才能观察出来]
    sigemptyset(&(act.sa_mask)); //把sa_mask设置为空集,意味在信号函数处理的过程中,不阻塞额外的信号[除了本信号]。
    act.sa_flags = 0; //初始化标识位
    //如果当前信号为时钟信号[时钟中断,可以实现定时器]
    //对于SIGALRM信号选择不让其重启慢系统调用,这是因为应用层可能需要该信号实现一些超时机制
    //避免进程无限期阻塞于慢系统调用中
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        //跨平台兼容,可能对于sunos 4.x系统接受到信号的默认行为都会重启系统调用
        act.sa_flags |= SA_INTERRUPT; //sunos 4.x
#endif
    } else {
#ifdef SA_RESTART
        //接受到信号时,会重启被打断的慢系统调用
        //具体表现就是,信号的回调函数只有当慢系统调用退出[目标事件发生],信号回调才会执行[进程接受到多个同类型信号只会保存最近收到的一个信号]且执行一次
        //但是不同的posix os对于可重启的系统调用也有自己的定义。比如Berkeley的实现从不自动重启select,有些实现从不重启accept和recvfrom等
        act.sa_flags |= SA_RESTART; //srv4 4.4BSD
#endif
    }
    //设置对signo信号的捕捉
    if(sigaction(signo,&act,&oact) < 0) {
        return (SIG_ERR);
    }
    return oact.sa_handler;
}

//返回二数的最大值
int Max(int a,int b) {
    return a>b?a:b;
}
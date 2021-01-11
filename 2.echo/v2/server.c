//
// Created by bytedance on 2020/11/3.
//

/**
 * 针对僵死进程做了优化,子进程退出时会给父进程发送SIGHUP信号。
 * 父进程通过捕捉该信号,对子进程进行回收,得到其退出结果以及统计信息后，子进程被彻底销毁
 */
//
// Created by bytedance on 2020/10/29.
//

#include "server.h"

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
   改进：
    --可以看到,注册了SIGCHLD的回调后,当客户端进程退出时,父进程收到SIGCHLD信号,执行回调,调用wait收集子进程的信息。
      僵尸进程消失。调用ps aux | grep --color "server_echo"命令。只能看到正处于S+状态[阻塞于accept调用]的父进程
      bytedance   83925   0.0  0.0  4276468    768 s003  S+    1:26下午   0:00.00 ./server_echo
   -- 那么这样是不是就已经完美了呢？并不是,我们知道信号并不是一个可靠的东西。当一个进程接受到一个信号时,它会阻塞同种信号直到信号回调处理完成。
      而在信号阻塞，操作系统内核永远只会为该进程保留每种信号的一个实例，多余的信号会被丢弃。所以当我们产生了比较多的子进程时，然后又并发退出。
      可能因为这个原因导致部分信号没被进程捕捉到，导致部分子进程还是处于僵尸状态。
      我同时启动了5个客户端,然后使用kill命令同时杀死这5个子进程。
          son progress pid 87725 collect done
          son progress pid 87672 collect done
          son progress pid 87619 collect done
      回调函数打印的日志显示,仅有三个子进程被回收。我们在使用 ps aux | grep --color "server_echo" 观察一下输出
      bytedance        87619   0.0  0.0        0      0 s003  Z+    1:39下午   0:00.00 (server_echo)
      bytedance        87566   0.0  0.0        0      0 s003  Z+    1:39下午   0:00.00 (server_echo)
      bytedance        86246   0.0  0.0  4276468    768 s003  S+    1:33下午   0:00.00 ./server_echo

      结果非常明显了,5个子进程中仍然有两个进程处于僵尸状态,说明内核发送给进程的信号在阻塞期间丢失了。因此我们还需要进一步优化
 */
sigfunc *when_son_progress_exit(int signnum) {
    int stat,pid;
    pid = wait(&stat);
    printf("son progress pid %d collect done\n",pid);
    return NULL;
}

/**
 *
 * @param signnum
 * @return
 *
 * 使用该函数后,效果如下
 * son progress pid 90600 collect done
   son progress pid 90547 collect done
   son progress pid 90494 collect done
   son progress pid 90440 collect done
   son progress pid 90733 collect done

   使用 ps aux | grep --color "server"命令
   bytedance        90435   0.0  0.0  4276468    768 s003  S+    2:01下午   0:00.00 ./server_echo
   可以看到,所有处于僵死状态的子进程都被回收了
   但是需要注意的是 waitpid 在默认情况下被调用时,只要进程组中包含尚未运行结束的子进程。就会一直阻塞，直到所有的子进程都运行结束时被唤醒。
   所以需要搭配 WNOHANG 选项,告知内核将其修改成非阻塞调用，每次调用都会返回，返回小于0的值就说明所有子进程都被收集干净了(不存在僵死进程了)。
   todo 返回0可能说明进程组中还有尚未运行结束的子进程[待验证]
 */
sigfunc *when_son_progress_exit_safe(int signnum) {
    int stat;
    pid_t pid;
    do {
        /**
         * pid_t waitpid(pid_t pid, int *stat, int options)
         * 参数解析
         * -- pid
         *   pid <- 1 等待进程组识别码为 pid 绝对值的任何子进程。
             pid = -1 等待任何子进程,相当于 wait()。
             pid = 0  等待进程组识别码与目前进程相同的任何子进程。
         *   pid > 1  等待任何子进程识别码为 pid 的子进程
         * -- stat *
         *   用于收集退出码信息的整型指针,配合其他函数可以获取子进程的退出信息
         * -- options
         *   配置项,指导waitpid的阻塞/非阻塞工作等
         * 返回值说明
         *   返回等待到的子进程的pid
         */
        // -1 意味着等待所有子进程
        // WNOHANG设置waitpid，当存在仍处于运行状态的子进程时不要阻塞[如果是阻塞调用,在处理信号回调时会阻塞主流程的代码[单线程]]
        // 当进程组中没有任何等待回收的子进程时,返回一个小于0的值,回调函数退出,父进程从accept处恢复调用
        pid = waitpid(-1,&stat,WNOHANG);
        if(pid > 0) {
            printf("son progress pid %d collect done\n",pid);
        }
    } while (pid > 0);
}

static sigfunc *when_pipe_break(int num) {
    ;
}

_Noreturn void server_echo() {

    signal_setter(SIGCHLD, when_son_progress_exit_safe); //注册子进程退出时的信号中断
    signal_setter(SIGPIPE, when_pipe_break); //注册子进程退出时的信号中断


    int listen_fd = Server(SERVER_PORT);
    if (listen_fd < 0) {
        ERR_PRINT("failed to listen")
    }
    int pid;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    bzero(&client_addr,client_addr_len);

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

//
// Created by bytedance on 2020/10/29.
//

#include <zconf.h>
#include "client.h"
#include "../../helper/helper.h"
#include "../../common/common.h"

_Noreturn static void str_echo(int fd,char* buffer,int buffer_len) {
    while (1) {
        //等待标准输入流中的数据
        char* result = fgets(buffer,buffer_len,stdin);
        //说明输入了EOF符
        if (NULL == result) {
            break;
        }
        printf("client say:%s",buffer);
        int n_write = writen(fd,buffer,buffer_len);
        //sleep(5);n_write = writen(fd,buffer,buffer_len);  测试写已经收到了rst分节套接字的代码
        if (n_write < 0) {
            ERR_PRINT("failed to write")
        }
        int n_read = read_once(fd,buffer,n_write);
        if (n_read < 0) {
            ERR_PRINT("failed to read")
        }
        if (n_read == 0) {
            ERR_PRINT("connection terminate from remote")
        }
        printf("server say:%s",buffer);
        fputs(buffer,stdout);
    }
}

sigfunc *when_pipe_break(int num){
    return NULL;
}

void cli_echo() {
    signal_setter(SIGPIPE, when_pipe_break); //注册子进程退出时的信号中断
    int connect_fd = Connect("127.0.0.1",SERVER_PORT);
    char buffer[255] = {'\0'};
    str_echo(connect_fd,buffer,sizeof(buffer));
    printf("exit because of EOF input\n");
    exit(0);
}
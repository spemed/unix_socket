//
// Created by bytedance on 2021/1/22.
//

#ifndef SELECT_CLIENT_SHOW_ALL_OPTIONS_H
#define SELECT_CLIENT_SHOW_ALL_OPTIONS_H
#include <netinet/tcp.h>
#include <ares_build.h>
#include <time.h>
#include <sys/time.h>

typedef union val {
    int i_val;
    long l_val;
    struct linger linger_val;
    struct timeval timeval_val;
} val;

static char *sock_str_flag(val *,int);
static char *sock_str_int(val *,int);
static char *sock_str_long(val *,int);
static char *sock_str_linger(val *,int);
static char *sock_str_timeval(val *,int);
void print_socket_options();

typedef struct socket_opts {
    const char* opt_str; //选项名的字符串形式
    int opt_level; //选项级别 存在socket,tcp,ipv4,ipv6,sctp等枚举值,分别描述不同维度的配置项(内核的socket设置,传输层设置,网络层设置)
    int opt_name; //选项名的枚举形式
    //这里是一个函数指针,可以通过函数指针来实现多态
    char *(*opt_handle)(val *,int); //负责处理不同规格选项[flag类型和值类型,flag类型用0和1标识开关,值类型又包括int型,long型,struct linger和struct timeval类型]
} socket_opts;

#endif //SELECT_CLIENT_SHOW_ALL_OPTIONS_H

//
// Created by bytedance on 2021/1/22.
//

#include "show_all_options.h"
#include "../helper/helper.h"

static char strres[128];

//校验flag类型,union val的值必须为int
static char *sock_str_flag(val* val,int len) {
    if (len != sizeof(int )) {
        snprintf(strres, sizeof(strres), "size (%d) not sizeof(int)", len);
    } else {
        snprintf(strres, sizeof(strres),"%s", (val->i_val == 0) ? "off" : "on");
    }
    return strres;
}

static char *sock_str_int(val* val,int len) {
    if (len != sizeof(int )) {
        snprintf(strres, sizeof(strres), "size (%d) not sizeof(int)", len);
    } else {
        snprintf(strres, sizeof(strres),"value=%d",val->i_val);
    }
    return strres;
}

static char *sock_str_long(val* val,int len) {
    return "123";
}

static char *sock_str_linger(val* val,int len) {
    if (len != sizeof(struct linger )) {
        snprintf(strres, sizeof(strres), "size (%d) not sizeof(struct linger)", len);
    } else {
        snprintf(strres, sizeof(strres),"value={status=%s,linger_time=%d}",val->linger_val.l_onoff == 1 ? "on" : "off",val->linger_val.l_linger);
    }
    return strres;
}

static char *sock_str_timeval(val* val,int len) {
    return "123";
}

socket_opts options[] = {
#ifdef SO_BROADCAST
    //socket level是否开启广播包选项,属于flag类型
    {"SO_BROADCAST",SOL_SOCKET,SO_BROADCAST,sock_str_flag},
#endif
#ifdef SO_DEBUG
    //socket level是否开启debug模式,属于flag类型
    {"SO_DEBUG",SOL_SOCKET,SO_DEBUG,sock_str_flag},
#endif
#ifdef SO_DONTROUTE
        //socket level是否绕过外出路由表查询,属于flag类型
    {"SO_DONTROUTE",SOL_SOCKET,SO_DONTROUTE,sock_str_flag},
#endif
#ifdef SO_ERROR
        //socket level获取错误并清除,属于值类型,但是只能get无法set
    {"SO_ERROR",SOL_SOCKET,SO_ERROR,sock_str_int},
#endif
#ifdef SO_KEEPALIVE
    //socket level是否开启tcp保活计时器[即tcp的心跳检测,非rfc标准],属于flag类型
    {"SO_KEEPALIVE",SOL_SOCKET,SO_KEEPALIVE,sock_str_flag},
#endif
    //socket level若有数据发送则延迟关闭,可get/set,属于值类型,值为linger
    /**
     * struct  linger {
            int     l_onoff;                 option on/off
            int     l_linger;                linger time
       };
       l_onoff 非0值为enable,0值为disable
       l_linger 延时的时间
      there are two ways to close the established connections
      正常方法：四次挥手,FIN|ACK|FIN|ACK,完成四次挥手后,主动申请关闭的一方[active close]会处于time_wait状态[防串包],并于2msl之后关闭[维持处于time_wait的连接需要占用操作系统的资源]
      异常关闭：直接关闭连接并向对端发送RST分节,连接会直接重置,主动关闭的一侧不会有time_wait状态。

      正常close的情况下,close会把套接字的引用计数减一,然后返回。减到0时,内核会试图把发送缓冲区中尚未发送的数据发送出去[保证数据可以发到对端],而后向对端发送FIN包。[对端执行read调用将会返回0]
      [阻塞套接字]
      当启动了l_onoff,且l_linger为0时,close直接返回,请求方将会抛弃发送缓冲区中未发送的数据,直接向对端发送RST分节,断开连接。[对端如果阻塞于select,poll等系统调用会返回err事件,如果阻塞于read系统调用将会返回-1。如果对一个已经收到RST分节的套接字进行write操作会触发SIGPIPE信号导致进程退出,进行read操作会返回ECONNECTIONRESET的错误]
      当启动了l_onoff,且l_linger为大于0的值时,close将会阻塞l_linger秒或者发送缓冲区的数据为空时返回,如果在规定秒数内可以发送完发送缓冲区中的数据，则进入正常的四次挥手流程。否则抛弃缓冲区中的所有数据并发送RST分节,然后close返回
      [非阻塞套接字]
      当启动了l_onoff,无论l_linger设置了多少秒,都会直接抛弃发送缓冲区中的数据,并向对端发送RST分节,close返回EWOULDBLOCK的err

      常有人通过设置SO_LINGER用于服务端需要短时间内断开大量连接的场景[主动关闭的一侧将会出现大量time_wait,占用服务器资源]。
      但是该做法并不被推荐,time_wait状态其实是对程序的一种保护[防止串包,确保数据的准确性]。正确做法应该是去改进通信协议。

      To linger or not, Linux edition
      So, what does this mean to the Linux network programmer?

      //todo 感觉有点前后矛盾
      //todo 写程序验证
      //阻塞套接字
        客户端:
            --> write 90000字节 --> 直接close
            --> 设置SO_LINGER -> 1,0 --> write 90000字节 --> close
            --> 设置SO_LINGER -> 1,5 --> write 90000字节 --> close
        服务端:
            accpet -> sleep(30) --> 打印读取到的数据的字节数
      //非阻塞套接字
        客户端:
            --> 设置SO_LINGER -> 1,5 --> write 90000字节 --> close
        服务端:
            accpet -> sleep(30) --> 打印读取到的数据的字节数

      If you’re using non-blocking sockets, note that calling close() may block the program
      Otherwise, the advice previously on BSD still holds good
    */
#ifdef SO_LINGER
    {"SO_LINGER",SOL_SOCKET,SO_LINGER,sock_str_linger},
#endif
#ifdef SO_OOBINLINE
    //socket level让接收到的带外数据继续在线残留,属于flag类型
    //todo 带外数据是否值tcp报文中被标记为urgent的报文呢?
    //todo 此外在线残留又是什么意思
    {"SO_OOBINLINE",SOL_SOCKET,SO_OOBINLINE,sock_str_flag},
#endif
#ifdef SO_RCVBUF
    //socket level接收缓冲区的大小[内核]
    {"SO_RCVBUF",SOL_SOCKET,SO_RCVBUF,sock_str_int},
#endif
#ifdef SO_SNDBUF
        //socket level发送缓冲区的大小[内核]
    {"SO_SNDBUF",SOL_SOCKET,SO_SNDBUF,sock_str_int},
#endif
#ifdef SO_RCVLOWAT
        //socket level接收缓冲区的低水位标记
        //todo 如果接收缓冲区中的数据大于低水位标记,select/poll都会返回可读事件
    {"SO_RCVLOWAT",SOL_SOCKET,SO_RCVLOWAT,sock_str_int},
#endif
#ifdef SO_SNDLOWAT
        //socket level发送缓冲区的低水位标记
        //todo 如果发送缓冲区中的数据大于低水位标记,select/poll都会返回可写事件
    {"SO_SNDLOWAT",SOL_SOCKET,SO_SNDLOWAT,sock_str_int},
#endif
#ifdef SO_REUSEADDR
    //socket level是否允许地址复用,flag类型
    {"SO_REUSEADDR",SOL_SOCKET,SO_REUSEADDR,sock_str_flag},
#endif
#ifdef SO_REUSEPORT
    //socket level是否允许端口复用,flag类型
    {"SO_REUSEPORT",SOL_SOCKET,SO_REUSEPORT,sock_str_flag},
#endif
#ifdef SO_TYPE
    //socket level取得套接字类型,值类型中的int类型,枚举值表示socket_type
    //todo socket()系统调用中,第二个参数
    //SOCK_STREAM 流式套接字
    //SOCK_DGRAM 用户数据报套接字
    //SOCK_RAW 原始套接字[工作于网络层,ping]
    {"SO_TYPE",SOL_SOCKET,SO_TYPE,sock_str_int},
#endif
#ifdef SO_USELOOPBACK
    //socket level路由套接字取得所发送数据的副本,flag类型
    {"SO_USELOOPBACK",SOL_SOCKET,SO_USELOOPBACK,sock_str_flag},
#endif
    {NULL,0,0,NULL}
};

void print_socket_options() {
    int fd = socket(AF_INET,SOCK_STREAM,0);
    val val;
    for (socket_opts *point=options; point->opt_str != NULL; ++point) {
//        switch (start->opt_level) {
//            case SOL_SOCKET:
//                break;
//            default:
//                printf("invalid opt_level\n");
//                exit(-1);
//        }
        printf("%s ",point->opt_str);
        if (NULL == point->opt_handle) {
            printf("undefined opt_handle\n");
        } else {
            socklen_t len = sizeof(val);
            int result = getsockopt(fd,point->opt_level,point->opt_name,&val,&len);
            if (-1 == result) {
                ERR_PRINT("failed to get socket option");
            } else {
                printf("%s\n",point->opt_handle(&val,len));
            }
        }
    }
}
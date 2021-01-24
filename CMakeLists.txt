cmake_minimum_required(VERSION 3.16)
project(select_client C)
set(CMAKE_C_STANDARD 99)
add_executable(
        so_linger_server
        main.c
        #        2.echo/v1/server.c
        #        2.echo/v1/server.h
        0.hello_socket/client.c
        0.hello_socket/client.h
        0.hello_socket/server.c
        0.hello_socket/server.h
        helper/helper.c
        helper/helper.h
        types/types.c
        types/types.h
        1.time_server/client.c
        1.time_server/client.h
        2.echo/v1/client.c
        2.echo/v1/client.h
        common/common.c
        common/common.h
        2.echo/v2/server.c
        2.echo/v2/server.h
        3.multiple_io/select/select_cli.c
        3.multiple_io/select/select_cli.h
        3.multiple_io/select/select_ser.c
        3.multiple_io/select/select_ser.h
        3.multiple_io/poll/poll_server.c
        3.multiple_io/poll/poll_server.h
        3.multiple_io/poll/poll_cli.c
        3.multiple_io/poll/poll_cli.h
        4.socket_option/show_all_options.c
        4.socket_option/show_all_options.h
        4.socket_option/so_linger.c
        4.socket_option/so_linger.h)
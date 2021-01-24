#include "1.time_server/server.h"
#include "0.hello_socket/server.h"
#include "3.multiple_io/poll/poll_cli.h"
#include "4.socket_option/so_linger.h"


int main() {
    server_handle(10000);
    return 0;
}


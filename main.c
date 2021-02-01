#include "1.time_server/server.h"
#include "0.hello_socket/server.h"
#include "3.multiple_io/poll/poll_cli.h"
#include "4.socket_option/so_linger.h"
#include "4.socket_option/show_all_options.h"


int main() {
    print_socket_options();
    return 0;
}


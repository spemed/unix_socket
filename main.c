#include <stdio.h>
#include "helper/helper.h"
#include "0.hello_socket/server.h"
#include "0.hello_socket/client.h"
#include "1.time_server/server.h"
#include "1.time_server/client.h"
#include "2.echo/v1/server.h"
#include "2.echo/v1/client.h"
#include "3.multiple_io/select/select_cli.h"
#include "3.multiple_io/select/select_ser.h"
#include "3.multiple_io/poll/poll_server.h"


int main() {
    poll_server(10000);
    return 0;
}


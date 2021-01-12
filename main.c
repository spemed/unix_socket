#include "1.time_server/server.h"
#include "3.multiple_io/poll/poll_cli.h"


int main() {
    poll_cli("127.0.0.1",10000);
    return 0;
}


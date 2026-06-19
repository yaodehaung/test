#include <stdio.h>
#include <stdlib.h>
#include <cache_controller.h>
#include <epoll_server.h>

#define DEFAULT_PORT 8080

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    register_cache_routes();

    return epoll_server_run(port);
}

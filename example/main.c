#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <cache_controller.h>
#include <epoll_server.h>

#define DEFAULT_PORT 8080

static void handle_signal(int signo)
{
    if (signo == SIGTRAP) {
        const char msg[] = "received SIGTRAP from kill -5, shutting down\n";

        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(128 + signo);
    }
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;

    // SIGKILL from kill -9 cannot be caught, blocked, or handled by a process.
    signal(SIGTRAP, handle_signal);

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    register_cache_routes();

    return epoll_server_run(port);
}

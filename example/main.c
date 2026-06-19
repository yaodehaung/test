#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <cache_controller.h>
#include <epoll_server.h>

#define DEFAULT_PORT 8080

static void handle_signal(int signo)
{
    if (signo == SIGHUP) {
        const char msg[] = "received SIGHUP from kill -1, continuing\n";

        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }

    if (signo == SIGTRAP) {
        const char msg[] = "received SIGTRAP from kill -5, shutting down\n";

        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(128 + signo);
    }
}

static void register_signal_handlers(void)
{
    // SIGKILL from kill -9 cannot be caught, blocked, or handled by a process.
    signal(SIGHUP, handle_signal);
    signal(SIGTRAP, handle_signal);
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    register_signal_handlers();
    register_cache_routes();

    return epoll_server_run(port);
}

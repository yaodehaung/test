#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <cache_controller.h>
#include <epoll_server.h>
#include <logger.h>

#define DEFAULT_PORT 8080
#ifndef NSIG
#define NSIG 64
#endif

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

    {
        const char msg[] = "received signal, shutting down\n";

        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(128 + signo);
    }
}

static void register_signal_handlers(void)
{
    for (int signo = 1; signo < NSIG; signo++) {
        // SIGKILL and SIGSTOP cannot be caught, blocked, or handled.
        if (signo == SIGKILL || signo == SIGSTOP) {
            continue;
        }

        signal(signo, handle_signal);
    }
}

static void configure_log_level(int argc, char **argv)
{
    const char *level = getenv("LOG_LEVEL");

    if (argc > 2) {
        level = argv[2];
    }

    if (!level) {
        LOG_INFO("log level: %s", log_level_name(log_get_level()));
        return;
    }

    if (!log_set_level_from_string(level)) {
        LOG_WARN("unknown log level '%s', using %s",
                 level, log_level_name(log_get_level()));
        return;
    }

    LOG_INFO("log level: %s", log_level_name(log_get_level()));
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    configure_log_level(argc, argv);
    register_signal_handlers();
    register_cache_routes();

    return epoll_server_run(port);
}

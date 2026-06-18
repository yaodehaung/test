#include "epoll_server.h"

#include "mini_express.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 64

typedef struct {
    int fd;
    char read_buf[4096];
    int read_idx;
} ClientState;

static int epoll_fd;

static void set_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static int write_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = write(fd, buf + sent, len - sent);

        if (n > 0) {
            sent += (size_t)n;
            continue;
        }

        if (n == -1 && errno == EINTR) {
            continue;
        }

        return -1;
    }

    return 0;
}

static void dispatch_to_framework(ClientState *client, const char *method,
                                  const char *path, const char *body)
{
    Response res;
    char header[512];

    app_dispatch_request(method, path, body, &res);

    snprintf(header, sizeof(header),
             "HTTP/1.1 %d OK\r\n"
             "Content-Type: application/json; charset=utf-8\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n\r\n",
             res.status_code, strlen(res.response_buf));

    if (write_all(client->fd, header, strlen(header)) == -1) {
        return;
    }
    write_all(client->fd, res.response_buf, strlen(res.response_buf));
}

int epoll_server_run(int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    ClientState *server_state;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 128);
    set_nonblocking(server_fd);

    epoll_fd = epoll_create1(0);
    server_state = calloc(1, sizeof(ClientState));
    server_state->fd = server_fd;
    ev.events = EPOLLIN;
    ev.data.ptr = server_state;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("100%% Pure C dependency-free web engine running on port %d...\n",
           port);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++) {
            ClientState *state = (ClientState *)events[i].data.ptr;

            if (state->fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);

                if (client_fd != -1) {
                    ClientState *c_state;

                    set_nonblocking(client_fd);
                    c_state = calloc(1, sizeof(ClientState));
                    c_state->fd = client_fd;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.ptr = c_state;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                }
            } else {
                char method[16] = {0};
                char path[256] = {0};
                char *body;

                while (1) {
                    int n = read(state->fd,
                                 state->read_buf + state->read_idx,
                                 sizeof(state->read_buf) -
                                     state->read_idx - 1);

                    if (n > 0) {
                        state->read_idx += n;
                        state->read_buf[state->read_idx] = '\0';
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        goto disconnect;
                    }
                }

                sscanf(state->read_buf, "%15s %255s", method, path);
                body = strstr(state->read_buf, "\r\n\r\n");
                if (body) {
                    body += 4;
                }

                dispatch_to_framework(state, method, path, body);

disconnect:
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, NULL);
                close(state->fd);
                free(state);
            }
        }
    }

    return 0;
}

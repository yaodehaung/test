#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "lib/mini_express.h"

#define PORT 8080
#define MAX_EVENTS 64
#define HASH_MAP_SIZE 1024

// ============================================================================
// In-memory hash map cache (Mini Redis style)
// ============================================================================
typedef struct HashNode {
    char *key; char *value; struct HashNode *next;
} HashNode;

typedef struct { HashNode *buckets[HASH_MAP_SIZE]; } HashMap;
HashMap *my_redis;

unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381; int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash % HASH_MAP_SIZE;
}

void hash_map_set(HashMap *map, const char *key, const char *value) {
    unsigned long idx = djb2_hash(key);
    HashNode *curr = map->buckets[idx];
    while (curr) {
        if (strcmp(curr->key, key) == 0) { free(curr->value); curr->value = strdup(value); return; }
        curr = curr->next;
    }
    HashNode *new_node = malloc(sizeof(HashNode));
    new_node->key = strdup(key); new_node->value = strdup(value);
    new_node->next = map->buckets[idx]; map->buckets[idx] = new_node;
}

const char* hash_map_get(HashMap *map, const char *key) {
    unsigned long idx = djb2_hash(key);
    HashNode *curr = map->buckets[idx];
    while (curr) { if (strcmp(curr->key, key) == 0) return curr->value; curr = curr->next; }
    return NULL;
}

// ============================================================================
// Minimal JSON field extractor
// ============================================================================
// Extract a quoted string field from a flat JSON object.
int json_get_string(const char *json, const char *field, char *output, size_t max_len) {
    if (!json) return 0;
    char target[64]; snprintf(target, sizeof(target), "\"%s\"", field);
    char *pos = strstr(json, target); if (!pos) return 0;
    pos = strchr(pos + strlen(target), ':'); if (!pos) return 0;
    pos = strchr(pos, '"'); if (!pos) return 0;
    char *start = pos + 1;
    char *end = strchr(start, '"'); if (!end) return 0;
    size_t len = end - start;
    if (len >= max_len) len = max_len - 1;
    strncpy(output, start, len); output[len] = '\0';
    return 1;
}

// ============================================================================
// API handlers and response writing
// ============================================================================
int write_all(int fd, const char *buf, size_t len) {
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

void handle_home(Request *req, Response *res) {
    (void)req;
    res_success(res, "Welcome to 100% Pure C Dependency-Free Express Engine!");
}

void handle_set_cache(Request *req, Response *res) {
    char key[64] = {0};
    char value[256] = {0};

    // Extract required string fields from the simple JSON body.
    int has_key = json_get_string(req->body, "key", key, sizeof(key));
    int has_val = json_get_string(req->body, "value", value, sizeof(value));

    if (has_key && has_val) {
        hash_map_set(my_redis, key, value);
        res_success(res, "Saved to Custom O(1) Cache completely dependency-free!");
    } else {
        res_error(res, 422, "Missing or invalid JSON fields: 'key' or 'value'");
    }
}

void handle_get_cache(Request *req, Response *res) {
    char key[64] = {0};
    int has_key = json_get_string(req->body, "key", key, sizeof(key));

    if (!has_key) {
        res_error(res, 400, "Please provide 'key' in request body");
        return;
    }

    const char *val = hash_map_get(my_redis, key);
    if (val) {
        char out[512];
        snprintf(out, sizeof(out), "{\"status\":\"hit\",\"key\":\"%s\",\"value\":\"%s\"}", key, val);
        res->status_code = 200;
        snprintf(res->response_buf, sizeof(res->response_buf), "%s", out);
    } else {
        res_error(res, 404, "Key not found in local hashmap database");
    }
}

// ============================================================================
// Linux epoll server core
// ============================================================================
int epoll_fd;
typedef struct { int fd; char read_buf[4096]; int read_idx; } ClientState;

void set_nonblocking(int fd) { fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK); }

void dispatch_to_framework(ClientState *client, const char *method, const char *path, const char *body) {
    Response res;

    app_dispatch_request(method, path, body, &res);

    // Build and send a minimal HTTP/1.1 JSON response.
    char header[512];
    sprintf(header, "HTTP/1.1 %d OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", res.status_code, strlen(res.response_buf));
    if (write_all(client->fd, header, strlen(header)) == -1) {
        return;
    }
    write_all(client->fd, res.response_buf, strlen(res.response_buf));
}

int main() {
    // Initialize the cache and register routes.
    my_redis = calloc(1, sizeof(HashMap));
    app_get("/", handle_home);
    app_post("/api/cache/set", handle_set_cache);
    app_get("/api/cache/get", handle_get_cache);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY };
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)); listen(server_fd, 128); set_nonblocking(server_fd);

    epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ClientState *server_state = calloc(1, sizeof(ClientState)); server_state->fd = server_fd;
    ev.events = EPOLLIN; ev.data.ptr = server_state; epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("100%% Pure C dependency-free web engine running on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            ClientState *state = (ClientState*)events[i].data.ptr;
            if (state->fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd != -1) {
                    set_nonblocking(client_fd);
                    ClientState *c_state = calloc(1, sizeof(ClientState)); c_state->fd = client_fd;
                    ev.events = EPOLLIN | EPOLLET; ev.data.ptr = c_state;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                }
            } else {
                // Read all currently available bytes on the edge-triggered socket.
                while (1) {
                    int n = read(state->fd, state->read_buf + state->read_idx, sizeof(state->read_buf) - state->read_idx - 1);
                    if (n > 0) { state->read_idx += n; state->read_buf[state->read_idx] = '\0'; }
                    else { if (errno == EAGAIN || errno == EWOULDBLOCK) break; goto disconnect; }
                }
                char method[16]={0}, path[256]={0}; sscanf(state->read_buf, "%15s %255s", method, path);
                char *body = strstr(state->read_buf, "\r\n\r\n"); if (body) body += 4;
                
                dispatch_to_framework(state, method, path, body);

            disconnect:
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state->fd, NULL); close(state->fd); free(state);
            }
        }
    }
    return 0;
} 

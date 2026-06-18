#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/core-net/epoll_server.h"
#include "lib/core/mini_express.h"
#include "lib/misc/hash_map.h"
#include "lib/misc/json_parser.h"

#define PORT 8080

static HashMap *my_redis;

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

int main() {
    // Initialize the cache and register routes.
    my_redis = calloc(1, sizeof(HashMap));
    app_get("/", handle_home);
    app_post("/api/cache/set", handle_set_cache);
    app_get("/api/cache/get", handle_get_cache);

    return epoll_server_run(PORT);
}

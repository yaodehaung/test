#include "cache_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <hash_map.h>
#include <json_parser.h>
#include <mini_express.h>

static HashMap *my_redis;

static void handle_set_cache(Request *req, Response *res)
{
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

static void handle_get_cache(Request *req, Response *res)
{
    char key[64] = {0};
    int has_key = json_get_string(req->body, "key", key, sizeof(key));

    if (!has_key) {
        res_error(res, 400, "Please provide 'key' in request body");
        return;
    }

    const char *val = hash_map_get(my_redis, key);
    if (val) {
        char out[512];
        snprintf(out, sizeof(out),
                 "{\"status\":\"hit\",\"key\":\"%s\",\"value\":\"%s\"}",
                 key, val);
        res->status_code = 200;
        snprintf(res->response_buf, sizeof(res->response_buf), "%s", out);
    } else {
        res_error(res, 404, "Key not found in local hashmap database");
    }
}

void register_cache_routes(void)
{
    my_redis = (HashMap *)calloc(1, sizeof(HashMap));
    app_post("/api/cache/set", handle_set_cache);
    app_get("/api/cache/get", handle_get_cache);
}

#include "mini_express.h"

#include <stdio.h>
#include <string.h>

#define MAX_ROUTES 20

/* Each rule maps one HTTP method and exact path to a handler callback. */
typedef struct {
    const char *method;
    const char *path;
    RouteHandler handler;
} RouteRule;

/* The tiny framework keeps a fixed-size in-memory route table. */
static RouteRule route_table[MAX_ROUTES];
static int route_count;

/* Store a route if there is capacity; extra routes are ignored. */
static void app_register(const char *method, const char *path,
                         RouteHandler handler)
{
    if (route_count >= MAX_ROUTES) {
        return;
    }

    route_table[route_count].method = method;
    route_table[route_count].path = path;
    route_table[route_count].handler = handler;
    route_count++;
}

void app_get(const char *path, RouteHandler handler)
{
    app_register("GET", path, handler);
}

void app_post(const char *path, RouteHandler handler)
{
    app_register("POST", path, handler);
}

void res_success(Response *res, const char *message)
{
    res->status_code = 200;
    snprintf(res->response_buf, sizeof(res->response_buf),
             "{\"status\":\"success\",\"message\":\"%s\"}", message);
}

void res_error(Response *res, int status_code, const char *error_message)
{
    res->status_code = status_code;
    snprintf(res->response_buf, sizeof(res->response_buf),
             "{\"status\":\"error\",\"error\":\"%s\"}", error_message);
}

void app_dispatch_request(const char *method, const char *path,
                          const char *body, Response *res)
{
    Request req = { method, path, body };

    /* Default to 404 until a route handler overwrites the response. */
    *res = (Response){ 404, "" };

    for (int i = 0; i < route_count; i++) {
        if (strcmp(method, route_table[i].method) == 0 &&
            strcmp(path, route_table[i].path) == 0) {
            route_table[i].handler(&req, res);
            return;
        }
    }

    res_error(res, 404, "Route URL not found");
}

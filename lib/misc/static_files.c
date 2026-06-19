#include "static_files.h"

#include <stdio.h>
#include <string.h>

#define STATIC_ROOT "public"
#define STATIC_BODY_MAX 12000

static const char *content_type_for_path(const char *path)
{
    const char *ext = strrchr(path, '.');

    if (!ext) {
        return "application/octet-stream";
    }

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html; charset=utf-8";
    }
    if (strcmp(ext, ".css") == 0) {
        return "text/css; charset=utf-8";
    }
    if (strcmp(ext, ".js") == 0) {
        return "application/javascript; charset=utf-8";
    }
    if (strcmp(ext, ".json") == 0) {
        return "application/json; charset=utf-8";
    }
    if (strcmp(ext, ".txt") == 0) {
        return "text/plain; charset=utf-8";
    }

    return "application/octet-stream";
}

static int is_safe_path(const char *path)
{
    return path && path[0] == '/' && strstr(path, "..") == NULL &&
           strchr(path, '\\') == NULL;
}

static int file_path_from_url(const char *url_path, char *out, size_t out_len)
{
    const char *path = url_path;
    const char *query;
    size_t len;

    if (!is_safe_path(url_path)) {
        return 0;
    }

    query = strchr(url_path, '?');
    len = query ? (size_t)(query - url_path) : strlen(url_path);

    if (len == 1 && url_path[0] == '/') {
        return snprintf(out, out_len, "%s/index.html", STATIC_ROOT) <
               (int)out_len;
    }

    if (path[0] == '/') {
        path++;
        len--;
    }

    if (len == 0 || len + sizeof(STATIC_ROOT) + 1 >= out_len) {
        return 0;
    }

    return snprintf(out, out_len, "%s/%.*s", STATIC_ROOT, (int)len, path) <
           (int)out_len;
}

int static_file_build_response(const char *url_path, char *out, size_t out_len)
{
    char file_path[512];
    char body[STATIC_BODY_MAX];
    FILE *file;
    size_t body_len;
    int header_len;

    if (!file_path_from_url(url_path, file_path, sizeof(file_path))) {
        return 0;
    }

    file = fopen(file_path, "rb");
    if (!file) {
        return 0;
    }

    body_len = fread(body, 1, sizeof(body), file);
    if (!feof(file)) {
        fclose(file);
        return 0;
    }
    fclose(file);

    header_len = snprintf(out, out_len,
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: %s\r\n"
                          "Content-Length: %zu\r\n"
                          "Connection: close\r\n\r\n",
                          content_type_for_path(file_path), body_len);
    if (header_len < 0 || (size_t)header_len + body_len >= out_len) {
        return 0;
    }

    memcpy(out + header_len, body, body_len);
    out[header_len + body_len] = '\0';
    return 1;
}

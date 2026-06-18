#include "json_parser.h"

#include <stdio.h>
#include <string.h>

int json_get_string(const char *json, const char *field, char *output,
                    size_t max_len)
{
    char target[64];
    char *pos;
    char *start;
    char *end;
    size_t len;

    if (!json) {
        return 0;
    }

    snprintf(target, sizeof(target), "\"%s\"", field);
    pos = strstr(json, target);
    if (!pos) {
        return 0;
    }

    pos = strchr(pos + strlen(target), ':');
    if (!pos) {
        return 0;
    }

    pos = strchr(pos, '"');
    if (!pos) {
        return 0;
    }

    start = pos + 1;
    end = strchr(start, '"');
    if (!end) {
        return 0;
    }

    len = (size_t)(end - start);
    if (len >= max_len) {
        len = max_len - 1;
    }

    strncpy(output, start, len);
    output[len] = '\0';
    return 1;
}

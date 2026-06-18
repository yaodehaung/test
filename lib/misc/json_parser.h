#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stddef.h>

/* Extract a quoted string field from a flat JSON object. */
int json_get_string(const char *json, const char *field, char *output,
                    size_t max_len);

#endif

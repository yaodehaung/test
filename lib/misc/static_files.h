#ifndef STATIC_FILES_H
#define STATIC_FILES_H

#include <stddef.h>

int static_file_build_response(const char *url_path, char *out,
                               size_t out_len);

#endif

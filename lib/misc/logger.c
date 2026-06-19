#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static LogLevel current_level = LOG_LEVEL_INFO;

static int equals_ignore_case(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a;
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca + ('a' - 'A'));
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb + ('a' - 'A'));
        }

        if (ca != cb) {
            return 0;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

void log_set_level(LogLevel level)
{
    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_NONE) {
        return;
    }

    current_level = level;
}

int log_set_level_from_string(const char *level)
{
    if (!level || *level == '\0') {
        return 0;
    }

    if (equals_ignore_case(level, "debug")) {
        log_set_level(LOG_LEVEL_DEBUG);
    } else if (equals_ignore_case(level, "info")) {
        log_set_level(LOG_LEVEL_INFO);
    } else if (equals_ignore_case(level, "warn") ||
               equals_ignore_case(level, "warning")) {
        log_set_level(LOG_LEVEL_WARN);
    } else if (equals_ignore_case(level, "error")) {
        log_set_level(LOG_LEVEL_ERROR);
    } else if (equals_ignore_case(level, "none") ||
               equals_ignore_case(level, "off")) {
        log_set_level(LOG_LEVEL_NONE);
    } else {
        return 0;
    }

    return 1;
}

LogLevel log_get_level(void)
{
    return current_level;
}

const char *log_level_name(LogLevel level)
{
    switch (level) {
    case LOG_LEVEL_DEBUG:
        return "debug";
    case LOG_LEVEL_INFO:
        return "info";
    case LOG_LEVEL_WARN:
        return "warn";
    case LOG_LEVEL_ERROR:
        return "error";
    case LOG_LEVEL_NONE:
        return "none";
    default:
        return "unknown";
    }
}

void log_message(LogLevel level, const char *fmt, ...)
{
    va_list args;

    if (level < current_level || current_level == LOG_LEVEL_NONE) {
        return;
    }

    fprintf(stderr, "[%s] ", log_level_name(level));

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
}

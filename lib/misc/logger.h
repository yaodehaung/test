#ifndef MISC_LOGGER_H
#define MISC_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE
} LogLevel;

void log_set_level(LogLevel level);
int log_set_level_from_string(const char *level);
LogLevel log_get_level(void);
const char *log_level_name(LogLevel level);
void log_message(LogLevel level, const char *fmt, ...);

#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

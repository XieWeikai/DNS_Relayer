#pragma once

#include <stdbool.h>

enum log_level {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

#define log_trace(...) log_log(LOG_LEVEL_TRACE, __VA_ARGS__)
#define log_debug(...) log_log(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(...)  log_log(LOG_LEVEL_INFO, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_LEVEL_WARN, __VA_ARGS__)
#define log_error(...) log_log(LOG_LEVEL_ERROR, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_LEVEL_FATAL, __VA_ARGS__)

void log_log(enum log_level level, const char *format, ...);

void log_set_level(enum log_level level);
void log_set_quiet(bool quiet);

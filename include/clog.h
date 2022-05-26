#pragma once

#include <stdbool.h>
#include <stdlib.h>

enum clog_level {
    CLOG_LEVEL_TRACE,
    CLOG_LEVEL_DEBUG,
    CLOG_LEVEL_INFO,
    CLOG_LEVEL_WARN,
    CLOG_LEVEL_ERROR,
    CLOG_LEVEL_FATAL,
};

#define log_trace(...) clog_log(CLOG_LEVEL_TRACE, __VA_ARGS__)
#define log_debug(...) clog_log(CLOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(...)  clog_log(CLOG_LEVEL_INFO, __VA_ARGS__)
#define log_warn(...)  clog_log(CLOG_LEVEL_WARN, __VA_ARGS__)
#define log_error(...) clog_log(CLOG_LEVEL_ERROR, __VA_ARGS__)
#define log_fatal(...) clog_log(CLOG_LEVEL_FATAL, __VA_ARGS__)

#define log_check(expr, ...)        \
    do {                            \
        if (!(expr)) {              \
            log_fatal(__VA_ARGS__); \
            exit(1);                \
        }                           \
    } while (0)

void clog_log(enum clog_level level, const char *format, ...);

void clog_set_level(enum clog_level level);
void clog_set_quiet(bool quiet);

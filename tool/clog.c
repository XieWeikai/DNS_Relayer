#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include "clog/clog.h"

static const char *log_level_str[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
};

static const char *level_colors[] = {
        "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

static const char *color_reset = "\x1b[0m";

static struct {
    bool quiet;
    enum clog_level level;
} log_state = {
        .quiet = false,
        .level = CLOG_LEVEL_INFO,
};

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void clog_log(enum clog_level level, const char *format, ...) {
    pthread_mutex_lock(&log_mutex);
    if (!log_state.quiet && level >= log_state.level) {
        fprintf(stderr, "%s%-5s%s ", level_colors[level], log_level_str[level], color_reset);
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    pthread_mutex_unlock(&log_mutex);
}

void clog_set_level(enum clog_level level) {
    log_state.level = level;
}

void clog_set_quiet(bool quiet) {
    log_state.quiet = quiet;
}


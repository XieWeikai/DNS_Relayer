#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

static struct {
    bool quiet;
    enum log_level level;
} log_state = {
        .quiet = false,
        .level = LOG_LEVEL_INFO,
};

static const char *log_level_str[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
};

static const char *level_colors[] = {
        "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

static const char *color_reset = "\x1b[0m";


void log_set_level(enum log_level level) {
    log_state.level = level;
}

void log_log(enum log_level level, const char *format, ...) {
    if (!log_state.quiet && level >= log_state.level) {
        fprintf(stderr, "%s%-5s%s ", level_colors[level], log_level_str[level], color_reset);
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void log_set_quiet(bool quiet) {
    log_state.quiet = quiet;
}


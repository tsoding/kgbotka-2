#include <stdlib.h>
#include <stdarg.h>

#include "./log.h"

Log log_to_handle(FILE *handle)
{
    return (Log) {
        .handle = handle
    };
}

void log_info(Log *log, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(log->handle, "[INFO] ");
    vfprintf(log->handle, fmt, args);
    fprintf(log->handle, "\n");
    va_end(args);
}

void log_warning(Log *log, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(log->handle, "[WARNING] ");
    vfprintf(log->handle, fmt, args);
    fprintf(log->handle, "\n");
    va_end(args);
}

void log_error(Log *log, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(log->handle, "[ERROR] ");
    vfprintf(log->handle, fmt, args);
    fprintf(log->handle, "\n");
    va_end(args);
}

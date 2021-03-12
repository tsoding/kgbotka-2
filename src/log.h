#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

typedef struct {
    FILE *handle;
} Log;

#if defined(__GNUC__) || defined(__clang__)
// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

Log log_to_handle(FILE *handle);
void log_info(Log *log, const char *fmt, ...) PRINTF_FORMAT(2, 3);
void log_warning(Log *log, const char *fmt, ...) PRINTF_FORMAT(2, 3);
void log_error(Log *log, const char *fmt, ...) PRINTF_FORMAT(2, 3);

#endif // LOG_H_

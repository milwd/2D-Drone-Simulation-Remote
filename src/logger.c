#include "logger.h"
#include <stdarg.h>
#include <time.h>

void log_message(const char *format, ...) {
    FILE *log_file = fopen("simple_log.txt", "a");
    if (!log_file) {
        fprintf(stderr, "Error: Could not open log file.\n");
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    // Print timestamp
    fprintf(log_file, "[%s] ", time_str);

    // Print the message
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");

    fclose(log_file);
}

#ifndef EXPML_LOG_H
#define EXPML_LOG_H

#include <stdarg.h>
#include <stdbool.h>

// Log level constants
#define LOG_LEVEL_EMERG   1  // System is unusable
#define LOG_LEVEL_ALERT   2  // Action must be taken immediately
#define LOG_LEVEL_CRIT    3  // Critical conditions
#define LOG_LEVEL_ERROR   4  // Error conditions
#define LOG_LEVEL_WARN    5  // Warning conditions
#define LOG_LEVEL_NOTICE  6  // Normal but significant condition
#define LOG_LEVEL_INFO    7  // Informational messages
#define LOG_LEVEL_DEBUG   8  // Debug-level messages

// Initialize logging system with file path and minimum level
bool Log_init(const char* log_file, int level);

// Close logging system and flush buffers
void Log_close(void);

// Set log level at runtime
void Log_setLevel(int level);

// Core logging function (not typically called directly)
void Log_write(int level, const char* fmt, ...);

// Convenience macros for each log level
#define LOG_EMERG(...)  Log_write(LOG_LEVEL_EMERG, __VA_ARGS__)
#define LOG_ALERT(...)  Log_write(LOG_LEVEL_ALERT, __VA_ARGS__)
#define LOG_CRIT(...)   Log_write(LOG_LEVEL_CRIT, __VA_ARGS__)
#define LOG_ERROR(...)  Log_write(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARN(...)   Log_write(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_NOTICE(...) Log_write(LOG_LEVEL_NOTICE, __VA_ARGS__)
#define LOG_INFO(...)   Log_write(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(...)  Log_write(LOG_LEVEL_DEBUG, __VA_ARGS__)

#endif // EXPML_LOG_H
#define _POSIX_C_SOURCE 200809L

#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>  // <--- ADDED: Required for va_list, va_start
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

// Global log state
static struct {
    FILE* file;
    int level;
    pthread_mutex_t lock;
    bool initialized;
} g_log = {
    .file = NULL,
    .level = LOG_LEVEL_INFO,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false
};

// Log level names
static const char* level_names[] = {
    "",
    "EMERG",
    "ALERT",
    "CRIT",
    "ERROR",
    "WARN",
    "NOTICE",
    "INFO",
    "DEBUG"
};

// Gets current timestamp in format: YYYY-MM-DD HH:MM:SS.mmm
static void get_timestamp(char* buf, size_t size) {
    struct timeval tv;
    struct tm tm_info;
    
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_info);
    
    size_t len = strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info);
    snprintf(buf + len, size - len, ".%03ld", tv.tv_usec / 1000);
}

bool Log_init(const char* log_file, int level) {
    pthread_mutex_lock(&g_log.lock);
    
    if (g_log.initialized && g_log.file) {
        fclose(g_log.file);
        g_log.file = NULL;
    }
    
    if (level < LOG_LEVEL_EMERG || level > LOG_LEVEL_DEBUG) {
        level = LOG_LEVEL_INFO;
    }
    
    g_log.level = level;
    
    // Open log file in append mode
    g_log.file = fopen(log_file, "a");
    if (!g_log.file) {
        pthread_mutex_unlock(&g_log.lock);
        return false;
    }
    
    // Disable buffering for immediate writes (safer for crashes, slower performance)
    // For high-performance logging, remove this line.
    setbuf(g_log.file, NULL);
    
    g_log.initialized = true;
    
    // We can't use LOG_INFO here recursively, so we write manually or just finish setup first.
    // It is safe to use fprintf here because we hold the lock and aren't calling Log_write.
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    fprintf(g_log.file, "%s [INFO] Logging initialized (level=%s, file=%s)\n", 
            timestamp, level_names[level], log_file);

    pthread_mutex_unlock(&g_log.lock);
    return true;
}

void Log_close(void) {
    pthread_mutex_lock(&g_log.lock);
    
    if (g_log.initialized && g_log.file) {
        // --- FIX: Direct fprintf to avoid Deadlock ---
        // Do NOT call LOG_INFO() here.
        fprintf(g_log.file, "[INFO] Logging shutdown\n");
        
        fflush(g_log.file);
        fclose(g_log.file);
        g_log.file = NULL;
    }
    
    g_log.initialized = false;
    pthread_mutex_unlock(&g_log.lock);
}

void Log_setLevel(int level) {
    pthread_mutex_lock(&g_log.lock);
    if (level >= LOG_LEVEL_EMERG && level <= LOG_LEVEL_DEBUG) {
        g_log.level = level;
    }
    pthread_mutex_unlock(&g_log.lock);
}

void Log_write(int level, const char* fmt, ...) {
    // Double-checked locking optimization
    if (!g_log.initialized || level > g_log.level) {
        return;
    }
    
    pthread_mutex_lock(&g_log.lock);
    
    if (!g_log.initialized || !g_log.file || level > g_log.level) {
        pthread_mutex_unlock(&g_log.lock);
        return;
    }
    
    if (level < LOG_LEVEL_EMERG || level > LOG_LEVEL_DEBUG) {
        pthread_mutex_unlock(&g_log.lock);
        return;
    }
    
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    fprintf(g_log.file, "%s [%s] ", timestamp, level_names[level]);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log.file, fmt, args);
    va_end(args);
    
    fprintf(g_log.file, "\n");
    
    // Flush critical errors immediately even if buffering is on
    if (level <= LOG_LEVEL_CRIT) {
        fflush(g_log.file);
    }
    
    pthread_mutex_unlock(&g_log.lock);
}
#define _POSIX_C_SOURCE 200809L

#include "LogViewer.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

// --- ANSI Color Definitions ---
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_GRAY    "\033[90m"
#define COLOR_BOLD    "\033[1m"

// Determines color based on log level string
static const char* get_level_color(const char* level) {
    if (strcmp(level, "EMERG") == 0)  return COLOR_BOLD COLOR_RED;
    if (strcmp(level, "ALERT") == 0)  return COLOR_BOLD COLOR_RED;
    if (strcmp(level, "CRIT") == 0)   return COLOR_BOLD COLOR_RED;
    if (strcmp(level, "ERROR") == 0)  return COLOR_RED;
    if (strcmp(level, "WARN") == 0)   return COLOR_YELLOW;
    if (strcmp(level, "NOTICE") == 0) return COLOR_MAGENTA;
    if (strcmp(level, "INFO") == 0)   return COLOR_GREEN; // <--- Fixed: Green for INFO
    if (strcmp(level, "DEBUG") == 0)  return COLOR_BLUE;  // <--- Fixed: Blue for DEBUG
    return COLOR_RESET;
}

// Helper: Converts string level to int constant
static int get_level_value(const char* level) {
    if (strcmp(level, "EMERG") == 0)  return LOG_LEVEL_EMERG;
    if (strcmp(level, "ALERT") == 0)  return LOG_LEVEL_ALERT;
    if (strcmp(level, "CRIT") == 0)   return LOG_LEVEL_CRIT;
    if (strcmp(level, "ERROR") == 0)  return LOG_LEVEL_ERROR;
    if (strcmp(level, "WARN") == 0)   return LOG_LEVEL_WARN;
    if (strcmp(level, "NOTICE") == 0) return LOG_LEVEL_NOTICE;
    if (strcmp(level, "INFO") == 0)   return LOG_LEVEL_INFO;
    if (strcmp(level, "DEBUG") == 0)  return LOG_LEVEL_DEBUG;
    return -1;
}

// Parses a raw log line, extracts the level, and prints it colorized
static void print_log_line(const char* line) {
    // Expected format: YYYY-MM-DD HH:MM:SS.mmm [LEVEL] Message
    
    // Find the level brackets
    const char* open_bracket = strchr(line, '[');
    const char* close_bracket = strchr(line, ']');

    // Validation: must have brackets, and close must be after open
    if (!open_bracket || !close_bracket || close_bracket <= open_bracket) {
        printf("%s\n", line); // Print raw if parsing fails
        return;
    }

    // 1. Print Timestamp (everything before [) in Gray
    size_t timestamp_len = open_bracket - line;
    if (timestamp_len > 0) {
        printf("%s%.*s%s", COLOR_GRAY, (int)timestamp_len, line, COLOR_RESET);
    }

    // 2. Extract Level String
    char level_str[16] = {0};
    size_t level_len = close_bracket - open_bracket - 1;
    if (level_len < sizeof(level_str)) {
        strncpy(level_str, open_bracket + 1, level_len);
        level_str[level_len] = '\0';
    }

    // 3. Print [LEVEL] in color
    const char* color = get_level_color(level_str);
    printf("[%s%s%s]", color, level_str, COLOR_RESET);

    // 4. Print Message (everything after ])
    printf("%s\n", close_bracket + 1);
}

// Checks if line meets the minimum log level requirement
static int should_show_line(const char* line, int min_level) {
    if (min_level == -1) return 1; // No filter

    const char* start = strchr(line, '[');
    const char* end = strchr(line, ']');
    if (!start || !end || end <= start) return 1;

    char level_str[16] = {0};
    size_t len = end - start - 1;
    if (len >= sizeof(level_str)) len = sizeof(level_str) - 1;
    strncpy(level_str, start + 1, len);

    int level_val = get_level_value(level_str);
    
    // Logic: If min_level is ERROR (3), we show ERROR(3), CRIT(2), ALERT(1).
    // So we show if level_val <= min_level
    return (level_val != -1 && level_val <= min_level);
}

// --- Main Viewer Functions ---

void LogViewer_show(const char* log_path, int tail_lines) {
    LogViewer_showFiltered(log_path, -1, tail_lines);
}

void LogViewer_showFiltered(const char* log_path, int min_level, int tail_lines) {
    FILE* f = fopen(log_path, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open log file: %s\n", log_path);
        return;
    }

    // Circular buffer to hold the last N lines
    char** buffer = calloc(tail_lines, sizeof(char*));
    int head = 0; // Current write position
    int count = 0; // Total lines currently in buffer
    
    char* line = NULL;
    size_t len = 0;

    while (getline(&line, &len, f) != -1) {
        // Strip newline
        line[strcspn(line, "\n")] = 0;
        
        if (should_show_line(line, min_level)) {
            if (buffer[head]) free(buffer[head]);
            buffer[head] = strdup(line);
            head = (head + 1) % tail_lines;
            if (count < tail_lines) count++;
        }
    }

    free(line);
    fclose(f);

    // Print buffer from oldest to newest
    int start_index = (count < tail_lines) ? 0 : head;
    for (int i = 0; i < count; i++) {
        int idx = (start_index + i) % tail_lines;
        if (buffer[idx]) {
            print_log_line(buffer[idx]);
            free(buffer[idx]);
        }
    }
    free(buffer);
}

void LogViewer_follow(const char* log_path) {
    FILE* f = fopen(log_path, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open log file: %s\n", log_path);
        return;
    }

    // Start at the end of the file
    fseek(f, 0, SEEK_END);
    
    printf("Following %s %s(Ctrl+C to stop)%s\n", log_path, COLOR_GRAY, COLOR_RESET);

    char* line = NULL;
    size_t len = 0;
    struct stat st;

    while (1) {
        long current_pos = ftell(f);
        
        // Read new lines
        while (getline(&line, &len, f) != -1) {
            line[strcspn(line, "\n")] = 0; // Strip newline
            print_log_line(line);
        }
        
        // Check for truncation (log rotation or restart)
        if (stat(log_path, &st) == 0) {
            if (st.st_size < current_pos) {
                printf("%s[Log file truncated/restarted]%s\n", COLOR_YELLOW, COLOR_RESET);
                fclose(f);
                f = fopen(log_path, "r");
                if (!f) break;
            }
        }
        
        clearerr(f);
        
        // Sleep 100ms
        struct timespec req = {0, 100000000};
        nanosleep(&req, NULL);
    }
    
    free(line);
    if (f) fclose(f);
}
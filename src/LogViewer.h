#ifndef EXPML_LOGVIEWER_H
#define EXPML_LOGVIEWER_H

// Show last N lines from log file with colors
void LogViewer_show(const char* log_path, int tail_lines);

// Follow log file in real-time (like tail -f)
void LogViewer_follow(const char* log_path);

// Show logs filtered by minimum level
void LogViewer_showFiltered(const char* log_path, int min_level, int tail_lines);

#endif // EXPML_LOGVIEWER_H
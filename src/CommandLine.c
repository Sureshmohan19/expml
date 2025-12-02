#include "CommandLine.h"
#include "Log.h"
#include "LogViewer.h"
#include "Storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void runTUI(const char* expml_dir);
#define VERSION "0.1.0"
#define PROGRAM_NAME "expml"

// Prints version information
static void printVersionFlag(void) { 
    printf("%s %s\n", PROGRAM_NAME, VERSION); 
}

// Prints help message
static void printHelpFlag(void) {
   printf("Usage: %s [OPTIONS] COMMAND [ARGS]...\n\n", PROGRAM_NAME);
   printf("Options:\n");
   printf("  --version  Show the version and exit.\n");
   printf("  --help     Show this message and exit.\n\n");
   printf("Commands:\n");
   printf("  run        Run an experiment TUI\n");
   printf("  logs       View experiment logs\n");
}

// Prints help for logs command
static void printLogsHelp(void) {
    printf("Usage: %s logs [OPTIONS]\n\n", PROGRAM_NAME);
    printf("View logs from experiment runs.\n\n");
    printf("Options:\n");
    printf("  -p, --path PATH    Path to run directory (default: latest-run)\n");
    printf("  -f, --follow       Follow log output (like tail -f)\n");
    printf("  -n, --tail N       Show last N lines (default: 50)\n");
    printf("  -l, --level LEVEL  Filter by minimum log level\n");
    printf("                     Levels: EMERG, ALERT, CRIT, ERROR, WARN, NOTICE, INFO, DEBUG\n");
    printf("  -h, --help         Show this help message\n");
}

// Converts level string to level constant
static int parseLogLevel(const char* level_str) {
    if (strcmp(level_str, "EMERG") == 0) return LOG_LEVEL_EMERG;
    if (strcmp(level_str, "ALERT") == 0) return LOG_LEVEL_ALERT;
    if (strcmp(level_str, "CRIT") == 0) return LOG_LEVEL_CRIT;
    if (strcmp(level_str, "ERROR") == 0) return LOG_LEVEL_ERROR;
    if (strcmp(level_str, "WARN") == 0) return LOG_LEVEL_WARN;
    if (strcmp(level_str, "NOTICE") == 0) return LOG_LEVEL_NOTICE;
    if (strcmp(level_str, "INFO") == 0) return LOG_LEVEL_INFO;
    if (strcmp(level_str, "DEBUG") == 0) return LOG_LEVEL_DEBUG;
    return -1; // Invalid level
}

// Handles the logs command
static CommandStatus handleLogsCommand(int argc, char** argv) {
    const char* run_path = NULL;
    int tail_lines = 50;
    int min_level = -1; // No filtering by default
    bool follow = false;
    
    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printLogsHelp();
            return CMD_EXIT;
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--follow") == 0) {
            follow = true;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires a path argument.\n", argv[i]);
                return CMD_ERROR;
            }
            run_path = argv[++i];
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--tail") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires a number argument.\n", argv[i]);
                return CMD_ERROR;
            }
            tail_lines = atoi(argv[++i]);
            if (tail_lines <= 0) {
                fprintf(stderr, "Error: tail lines must be positive.\n");
                return CMD_ERROR;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--level") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires a level argument.\n", argv[i]);
                return CMD_ERROR;
            }
            min_level = parseLogLevel(argv[++i]);
            if (min_level == -1) {
                fprintf(stderr, "Error: Invalid log level '%s'.\n", argv[i]);
                fprintf(stderr, "Valid levels: EMERG, ALERT, CRIT, ERROR, WARN, NOTICE, INFO, DEBUG\n");
                return CMD_ERROR;
            }
        }
        else {
            fprintf(stderr, "Error: Unknown option '%s'.\n", argv[i]);
            fprintf(stderr, "Try '%s logs --help' for usage.\n", PROGRAM_NAME);
            return CMD_ERROR;
        }
    }
    
    // Determine log file path
    char log_file[1024];
    if (run_path) {
        // Use provided path
        snprintf(log_file, sizeof(log_file), "%s/debug.log", run_path);
    } else {
        // Use latest-run
        char* latest_run = Storage_findLatestRun("expml_runs");
        if (!latest_run) {
            fprintf(stderr, "Error: Could not find latest run in expml_runs/\n");
            fprintf(stderr, "Hint: Use -p to specify a run directory.\n");
            return CMD_ERROR;
        }
        snprintf(log_file, sizeof(log_file), "%s/debug.log", latest_run);
        free(latest_run);
    }
    
    // Check if log file exists
    if (access(log_file, F_OK) != 0) {
        fprintf(stderr, "Error: Log file not found: %s\n", log_file);
        fprintf(stderr, "Hint: Logging may not be enabled for this run.\n");
        return CMD_ERROR;
    }
    
    // Execute appropriate viewer function
    if (follow) {
        LogViewer_follow(log_file);
    } else if (min_level != -1) {
        LogViewer_showFiltered(log_file, min_level, tail_lines);
    } else {
        LogViewer_show(log_file, tail_lines);
    }
    
    return CMD_SUCCESS;
}

// Parses and executes commands
static CommandStatus parseCommand(int argc, char** argv) {
   if (argc < 2) { printHelpFlag(); return CMD_EXIT; }
   const char* command = argv[1];
   
   if (strcmp(command, "--version") == 0) { printVersionFlag(); return CMD_EXIT; }
   if (strcmp(command, "--help") == 0)    { printHelpFlag();    return CMD_EXIT; }
   
   if (strcmp(command, "run") == 0) {
      const char* expml_dir = "expml_runs";
      if (argc > 2 && strcmp(argv[2], "-p") == 0) {
         if (argc < 4) { fprintf(stderr, "Error: -p requires a path argument.\n"); return CMD_ERROR; }
         expml_dir = argv[3];
      }  
      runTUI(expml_dir);
      return CMD_SUCCESS;
   }
   
   if (strcmp(command, "logs") == 0) {
      return handleLogsCommand(argc, argv);
   }
   
   fprintf(stderr, "Usage: %s [OPTIONS] COMMAND [ARGS]...\n", PROGRAM_NAME);
   fprintf(stderr, "Try '%s --help' for help.\n\n", PROGRAM_NAME);
   fprintf(stderr, "Error: No such command '%s'.\n", command);
   return CMD_ERROR;
}

// Main entry point for command line interface
int CommandLine_run(int argc, char** argv) {
    switch (parseCommand(argc, argv)) {
        case CMD_SUCCESS: return 0;
        case CMD_EXIT:    return 0;
        case CMD_ERROR:   return 1;
        default:          return 1;
    }
}
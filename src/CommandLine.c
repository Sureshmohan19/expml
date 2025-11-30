#include "CommandLine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void runTUI(const char* expml_dir);
#define VERSION "0.1.0"
#define PROGRAM_NAME "expml"

static void printVersionFlag(void) { printf("%s %s\n", PROGRAM_NAME, VERSION); }
static void printHelpFlag(void) {
   printf("Usage: %s [OPTIONS] COMMAND [ARGS]...\n\n", PROGRAM_NAME);
   printf("Options:\n");
   printf("  --version  Show the version and exit.\n");
   printf("  --help     Show this message and exit.\n\n");
   printf("Commands:\n");
   printf("  run        Run an experiment TUI\n");
}

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
   fprintf(stderr, "Usage: %s [OPTIONS] COMMAND [ARGS]...\n", PROGRAM_NAME);
   fprintf(stderr, "Try '%s --help' for help.\n\n", PROGRAM_NAME);
   fprintf(stderr, "Error: No such command '%s'.\n", command);
   return CMD_ERROR;
}

int CommandLine_run(int argc, char** argv) {
    switch (parseCommand(argc, argv)) {
        case CMD_SUCCESS: return 0;
        case CMD_EXIT:    return 0;
        case CMD_ERROR:   return 1;
        default:          return 1;
    }
}
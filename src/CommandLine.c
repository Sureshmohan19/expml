#define _POSIX_C_SOURCE 200809L // Required for access() and F_OK

#include "CommandLine.h"
#include "Storage.h"
#include "Terminal.h"
#include "ScreenManager.h"
#include "FunctionBar.h"
#include "Panel.h"
#include "RunPanel.h" 
#include "SystemPanel.h"
#include "MetricsPanel.h"
#include "DataLoader.h"

#include <math.h>
#include <cjson/cJSON.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION "0.1.0"
#define PROGRAM_NAME "expml"

static void printVersionFlag(void) {
   printf("%s %s\n", PROGRAM_NAME, VERSION);
}

static void printHelpFlag(void) {
   printf("Usage: %s [OPTIONS] COMMAND [ARGS]...\n\n", PROGRAM_NAME);
   printf("Options:\n");
   printf("  --version  Show the version and exit.\n");
   printf("  --help     Show this message and exit.\n\n");
   printf("Commands:\n");
   printf("  run        Run an experiment TUI\n");
   printf("  list       List experiments\n");
}

// Context for the refresh callback
typedef struct {
    char* run_path;
    Panel* runPanel;
    Panel* metricsPanel;
    Panel* systemPanel;
    FunctionBar* funcBar;
} AppContext;

// callback function
static void on_refresh(void* userdata) {
    AppContext* ctx = (AppContext*)userdata;
    
    // 1. Reload Metrics (Stream)
    // Note: In a production app, we would track file offset to avoid re-reading the whole file.
    // For v0.0.1, re-reading is acceptable for small experiments.
    DataLoader_loadMetrics(ctx->run_path, ctx->metricsPanel, ctx->systemPanel);
    
    // 2. Reload Static Data (Summary/Config updates)
    // We re-read summary.json to get the latest status/runtime
    RunSummary* summary = Storage_readSummary(ctx->run_path);
    RunConfig* config = Storage_readConfig(ctx->run_path);
    RunMetadata* meta = Storage_readMetadata(ctx->run_path);

    if (summary || config || meta) {
         RunPanel_setData(ctx->runPanel, config, meta, summary);
    }

    // 3. Update Status Bar Live
    if (summary) {
        // Using '|' instead of bullet char to avoid artifacts
        FunctionBar_setContext(ctx->funcBar, 
            "State: %s | Runtime: %.0fs | Step: %d", 
            summary->status ? summary->status : "UNKNOWN", 
            summary->runtime, 
            summary->step
        );
    }

    // Cleanup temporary structs
    Storage_freeRunSummary(summary);
    Storage_freeRunConfig(config);
    Storage_freeRunMetadata(meta);
}

static void runTUI(const char* expml_dir) {
    printf("DEBUG: Searching for latest run in directory: '%s'\n", expml_dir);

   // 1. Find Run Path
   char* run_path = Storage_findLatestRun(expml_dir);
   if (!run_path) {
       fprintf(stderr, "ERROR: Could not resolve 'latest-run' in '%s'\n", expml_dir);
       if (access(expml_dir, F_OK) != 0) {
           fprintf(stderr, "       Directory '%s' does not exist.\n", expml_dir);
       }
       return;
   } 

   // 3. Initialize Terminal
   Terminal_init(true);
   // We start with a generic header; on_refresh will update it via panels later if needed
   ScreenManager* sm = ScreenManager_new("Waiting for data...", 1.0); 

   const char* keys[] = {"F1", "h", "q", "F10", NULL};
   const char* labels[] = {"Help", "Help", "Quit", "Quit", NULL};

   FunctionBar* fb = FunctionBar_new(keys, labels);
   ScreenManager_setFunctionBar(sm, fb);
   
   // 5. Add Panels

   // LEFT: Run Overview (REAL DATA)
   Panel* runPanel = RunPanel_new(0, 0, 0, 0);
   // Populate it!
   ScreenManager_addPanel(sm, runPanel, 45);

   // MIDDLE: Metrics (Still Fake for now)
   Panel* metricsPanel = MetricsPanel_new(0, 0, 0, 0);
   ScreenManager_addPanel(sm, metricsPanel, 0);

   // RIGHT: System Metrics (Still Fake for now)
   Panel* systemPanel = SystemPanel_new(0, 0, 0, 0);
   ScreenManager_addPanel(sm, systemPanel, 35);

    // 5. SETUP LIVE REFRESH
   AppContext ctx;
   ctx.run_path = run_path;
   ctx.runPanel = runPanel;
   ctx.metricsPanel = metricsPanel;
   ctx.systemPanel = systemPanel;
   ctx.funcBar = fb;

   ScreenManager_setRefreshCallback(sm, on_refresh, &ctx);
   
   // Trigger one load immediately
   on_refresh(&ctx);

   // 6. Run Loop
   ScreenManager_run(sm);

   // ... (Cleanup) ...
   ScreenManager_delete(sm);
   Terminal_done(); 
   free(run_path);
}

static CommandStatus parseCommand(int argc, char** argv) {
   if (argc < 2) {
      printHelpFlag();
      return CMD_EXIT;
}

   const char* command = argv[1];

   // Handle --version and --help as first argument
   if (strcmp(command, "--version") == 0) {
      printVersionFlag();
      return CMD_EXIT;
   }

   if (strcmp(command, "--help") == 0) {
      printHelpFlag();
      return CMD_EXIT;
   }

   // Handle commands
   if (strcmp(command, "run") == 0) {
      // Check for -p option
      const char* expml_dir = "expml_runs"; // Default directory
      
      if (argc > 2 && strcmp(argv[2], "-p") == 0) {
         if (argc < 4) {
            fprintf(stderr, "Error: -p requires a path argument.\n");
            return CMD_ERROR;
         }
         expml_dir = argv[3];
      }
      
      // Launch the TUI
      runTUI(expml_dir);
      return CMD_SUCCESS;
   }

   if (strcmp(command, "list") == 0) {
      printf("Command 'list' will be implemented soon.\n");
      return CMD_SUCCESS;
   }

   // Unknown command
   fprintf(stderr, "Usage: %s [OPTIONS] COMMAND [ARGS]...\n", PROGRAM_NAME);
   fprintf(stderr, "Try '%s --help' for help.\n\n", PROGRAM_NAME);
   fprintf(stderr, "Error: No such command '%s'.\n", command);
   return CMD_ERROR;
}

int CommandLine_run(int argc, char** argv) {
   CommandStatus status = parseCommand(argc, argv);
   
   switch (status) {
      case CMD_SUCCESS:
         return 0;
      case CMD_ERROR:
         return 1;
      case CMD_EXIT:
         return 0;
      default:
         return 1;
   }
}

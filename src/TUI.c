#define _POSIX_C_SOURCE 200809L

#include "TUI.h"
#include "Log.h"
#include "Panel.h"
#include "Storage.h"
#include "Terminal.h"
#include "Constants.h"
#include "RunPanel.h"
#include "DataLoader.h"
#include "FunctionBar.h"
#include "SystemPanel.h"
#include "MetricsPanel.h"
#include "ScreenManager.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cjson/cJSON.h>

typedef struct {
    char* run_path;
    Panel* runPanel;
    Panel* metricsPanel;
    Panel* systemPanel;
    FunctionBar* funcBar;
    ScreenManager* sm;
} AppContext;

// Refresh callback - reloads data from storage
static void on_refresh(void* userdata) {
   AppContext* ctx = (AppContext*)userdata;

   int saved_metrics_selection = Panel_getSelectedIndex(ctx->metricsPanel);
   DataLoader_loadMetrics(ctx->run_path, ctx->metricsPanel, ctx->systemPanel);
   Panel_setSelected(ctx->metricsPanel, saved_metrics_selection);

   RunSummary* summary = Storage_readSummary(ctx->run_path);
   RunConfig* config = Storage_readConfig(ctx->run_path);
   RunMetadata* meta = Storage_readMetadata(ctx->run_path);

   if (summary || config || meta) { RunPanel_setData(ctx->runPanel, config, meta, summary); }
   
   if (summary) {
        ScreenManager_setHeaderStatus(ctx->sm, summary->status);
        ScreenManager_setHeaderRuntime(ctx->sm, summary->runtime);

        const char* r_name = (meta && meta->run_name) ? meta->run_name : "Unknown";
        const char* status = summary->status ? summary->status : "UNKNOWN";

        FunctionBar_setContext(ctx->funcBar, 
            " Run: %s | State: %s | Runtime: %.0fs | Step: %d", 
            r_name,
            status, 
            summary->runtime, 
            summary->step
        );

        // If the experiment is done, there is no need to reload files every second.
        // This saves CPU and prevents selection glitches.
        if (strcmp(status, "FINISHED") == 0 || 
            strcmp(status, "FAILED") == 0 || 
            strcmp(status, "CRASHED") == 0 || 
            strcmp(status, "STOPPED") == 0) {
            
            // Passing NULL disables the timer in ScreenManager
            ScreenManager_setRefreshCallback(ctx->sm, NULL, NULL);
        }
   }
   
   Storage_freeRunSummary(summary);
   Storage_freeRunConfig(config);
   Storage_freeRunMetadata(meta);
}

// Main TUI entry point
void runTUI(const char* expml_dir) {
   char* run_path = Storage_findLatestRun(expml_dir);
   if (!run_path) {
       fprintf(stderr, "ERROR: Could not resolve 'latest-run' in '%s'\n", expml_dir);
       if (access(expml_dir, F_OK) != 0) {
           fprintf(stderr, "       Directory '%s' does not exist.\n", expml_dir);
       }
       return;
   }
   
   // 1. Initialize Logging FIRST
   // We act silently if it fails to avoid breaking the TUI layout with stderr output
   char log_path[1024];
   snprintf(log_path, sizeof(log_path), "%s/debug.log", run_path);
   Log_init(log_path, LOG_LEVEL_INFO);
   
   LOG_INFO("--- TUI Session Started ---");
   LOG_INFO("Run Path: %s", run_path);
   
   // 2. Load initial data
   RunMetadata* meta = Storage_readMetadata(run_path);
   RunSummary* summary = Storage_readSummary(run_path);
   
   char header_text[256];
   if (meta && meta->run_name) {
       snprintf(header_text, sizeof(header_text), "%s", meta->run_name);
   } else if (summary && summary->status && strcmp(summary->status, "FINISHED") == 0) {
       snprintf(header_text, sizeof(header_text), "Experiment Snapshot");
   } else {
       snprintf(header_text, sizeof(header_text), "Running Experiment");
   }

   // 3. Initialize Terminal (ncurses)
   // It is crucial that NO text is printed to stdout/stderr before this line
   Terminal_init(true);
   
   ScreenManager* sm = ScreenManager_new(header_text, 1.0);

   Storage_freeRunMetadata(meta);
   Storage_freeRunSummary(summary);

   const char* keys[] = {"h", "q", NULL};
   const char* labels[] = {"Help", "Quit", NULL};

   FunctionBar* fb = FunctionBar_new(keys, labels);
   ScreenManager_setFunctionBar(sm, fb);

   // 4. Create Panels
   // Widths: Left=35, Right=35, Middle=Auto(0)
   Panel* runPanel = RunPanel_new(0, 0, 0, 0);
   ScreenManager_addPanel(sm, runPanel, SIDEBAR_DEFAULT_WIDTH);

   Panel* metricsPanel = MetricsPanel_new(0, 0, 0, 0);
   ScreenManager_addPanel(sm, metricsPanel, 0);
   
   Panel* systemPanel = SystemPanel_new(0, 0, 0, 0);
   ScreenManager_addPanel(sm, systemPanel, SIDEBAR_DEFAULT_WIDTH);

   AppContext ctx;
   ctx.run_path = run_path;
   ctx.runPanel = runPanel;
   ctx.metricsPanel = metricsPanel;
   ctx.systemPanel = systemPanel;
   ctx.funcBar = fb;
   ctx.sm = sm; 

   // 5. Start Loop
   ScreenManager_setRefreshCallback(sm, on_refresh, &ctx);
   
   // Trigger one refresh before loop to populate data immediately
   on_refresh(&ctx);
   
   ScreenManager_run(sm);

   // 6. Cleanup
   ScreenManager_delete(sm);
   Terminal_done(); // Restore terminal
   
   LOG_INFO("TUI Session Ended");
   Log_close(); // Close log file
   
   free(run_path);
}
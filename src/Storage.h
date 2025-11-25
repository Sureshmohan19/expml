#ifndef EXPML_STORAGE_H
#define EXPML_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

// Forward declare cJSON to avoid exposing the library headers globally
typedef struct cJSON cJSON;

/*
 * RunConfig - Represents config.json
 * Holds hyperparameters.
 * TODO: Add support for nested configuration objects in future versions.
 */
typedef struct RunConfig_ {
    cJSON* json;           // The full JSON object (key-value pairs)
} RunConfig;

/*
 * RunMetadata - Represents metadata.json
 * Holds static system info captured at startup.
 */
typedef struct RunMetadata_ {
    char* run_id;          // "id"
    char* run_name;        // "name"
    char* user;            // "user"
    char* host;            // "host"
    char* os;              // "os"
    char* python;          // "python"
    char* gpu_name;        // "gpu_name"
    char* disk_total;      // "disk_total"
    char* ram_total;       // "ram_total"
    char* command;         // "command"
    int cpu_count;         // "cpu_count"
    int gpu_count;         // "gpu_count"
} RunMetadata;

/*
 * RunSummary - Represents summary.json
 * Holds the latest/best state of the run.
 */
typedef struct RunSummary_ {
    char* status;          // "status": RUNNING, FINISHED, etc.
    double runtime;        // "_runtime"
    double timestamp;      // "_timestamp"
    int step;              // "_step"
    int epoch;             // "epoch"
    cJSON* json;           // The full JSON object (contains metrics like loss/acc)
} RunSummary;

/*
 * MetricEntry - Represents one line in metrics.jsonl
 */
typedef struct MetricEntry_ {
    int step;              // "_step"
    double timestamp;      // "_timestamp"
    
    // The raw JSON data for this step.
    // Contains "loss", "accuracy", "system/cpu", etc.
    // TODO: Add specific parsing for "histogram" objects in future versions.
    cJSON* json;           
} MetricEntry;


// --- API Functions ---

/*
 * Find the latest run directory (via "latest-run" symlink)
 * Returns: Malloc'd string path, or NULL.
 */
char* Storage_findLatestRun(const char* expml_dir);

/*
 * Readers for specific files
 * All return NULL on error/missing file.
 * All return structs that must be freed with the corresponding free function.
 */
RunConfig* Storage_readConfig(const char* run_dir);
RunMetadata* Storage_readMetadata(const char* run_dir);
RunSummary* Storage_readSummary(const char* run_dir);

/*
 * Streaming Reader for metrics.jsonl
 */
void* Storage_openMetrics(const char* run_dir);
MetricEntry* Storage_readNextMetric(void* handle);
void Storage_closeMetrics(void* handle);

/*
 * Memory Management
 */
void Storage_freeRunConfig(RunConfig* config);
void Storage_freeRunMetadata(RunMetadata* meta);
void Storage_freeRunSummary(RunSummary* summary);
void Storage_freeMetricEntry(MetricEntry* entry);

#endif

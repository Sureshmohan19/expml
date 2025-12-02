#ifndef EXPML_STORAGE_H
#define EXPML_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct cJSON cJSON;

typedef struct RunConfig_ {
    cJSON* json;
} RunConfig;

typedef struct RunMetadata_ {
    char* run_id;
    char* run_name;
    char* user;
    char* host;
    char* os;
    char* python;
    char* gpu_name;
    char* disk_total;
    char* ram_total;
    char* command;
    int cpu_count;
    int gpu_count;
} RunMetadata;

typedef struct RunSummary_ {
    char* status;
    double runtime;
    double timestamp;
    int step;
    int epoch;
    cJSON* json;
} RunSummary;

typedef struct MetricEntry_ {
    int step;
    double timestamp;
    cJSON* json;           
} MetricEntry;

// Finds and returns the path to the most recent run directory in the given expml directory
char* Storage_findLatestRun(const char* expml_dir);

// Reads and returns the configuration for a specific run
RunConfig* Storage_readConfig(const char* run_dir);

// Reads and returns the metadata for a specific run
RunMetadata* Storage_readMetadata(const char* run_dir);

// Reads and returns the summary information for a specific run
RunSummary* Storage_readSummary(const char* run_dir);

// Opens the metrics file for sequential reading and returns an opaque handle
void* Storage_openMetrics(const char* run_dir);

// Reads the next metric entry from an open metrics handle, or NULL if no more entries
MetricEntry* Storage_readNextMetric(void* handle);

// Closes an open metrics handle and releases associated resources
void Storage_closeMetrics(void* handle);

// Frees memory allocated for a RunConfig structure
void Storage_freeRunConfig(RunConfig* config);

// Frees memory allocated for a RunMetadata structure
void Storage_freeRunMetadata(RunMetadata* meta);

// Frees memory allocated for a RunSummary structure
void Storage_freeRunSummary(RunSummary* summary);

// Frees memory allocated for a MetricEntry structure
void Storage_freeMetricEntry(MetricEntry* entry);

#endif

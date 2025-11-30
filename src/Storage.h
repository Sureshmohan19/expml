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

char* Storage_findLatestRun(const char* expml_dir);

RunConfig* Storage_readConfig(const char* run_dir);
RunMetadata* Storage_readMetadata(const char* run_dir);
RunSummary* Storage_readSummary(const char* run_dir);

void* Storage_openMetrics(const char* run_dir);
MetricEntry* Storage_readNextMetric(void* handle);
void Storage_closeMetrics(void* handle);

void Storage_freeRunConfig(RunConfig* config);
void Storage_freeRunMetadata(RunMetadata* meta);
void Storage_freeRunSummary(RunSummary* summary);
void Storage_freeMetricEntry(MetricEntry* entry);

#endif

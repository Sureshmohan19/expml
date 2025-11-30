#define _POSIX_C_SOURCE 200809L 

#include "Storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>

typedef struct MetricsHandle_ {
    FILE* file;
    char* line_buffer;
    size_t buffer_size;
} MetricsHandle;

static char* readFileToString(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) { fclose(f); return NULL; }

    char* content = malloc(size + 1);
    if (!content) { fclose(f); return NULL; }
    
    size_t read = fread(content, 1, size, f);
    content[read] = '\0';
    fclose(f);
    return content;
}

static char* buildPath(const char* dir, const char* filename) {
    size_t len = strlen(dir) + strlen(filename) + 2; 
    char* path = malloc(len);
    if (!path) return NULL;
    snprintf(path, len, "%s/%s", dir, filename);
    return path;
}

static char* getJsonString(cJSON* root, const char* key, const char* def) {
    cJSON* item = cJSON_GetObjectItem(root, key);
    if (item && cJSON_IsString(item)) { return strdup(item->valuestring); }
    return def ? strdup(def) : NULL;
}

static int getJsonInt(cJSON* root, const char* key, int def) {
    cJSON* item = cJSON_GetObjectItem(root, key);
    if (item && cJSON_IsNumber(item)) { return item->valueint; }
    return def;
}

static double getJsonDouble(cJSON* root, const char* key, double def) {
    cJSON* item = cJSON_GetObjectItem(root, key);
    if (item && cJSON_IsNumber(item)) { return item->valuedouble; }
    return def;
}

char* Storage_findLatestRun(const char* expml_dir) {
    char* symlink_path = buildPath(expml_dir, "latest-run");
    if (!symlink_path) return NULL;

    struct stat st;
    if (lstat(symlink_path, &st) != 0 || !S_ISLNK(st.st_mode)) {
        free(symlink_path);
        return NULL;
    }
    
    char target[1024];
    ssize_t len = readlink(symlink_path, target, sizeof(target) - 1);
    free(symlink_path);
    
    if (len < 0) return NULL;
    target[len] = '\0';
    if (target[0] == '/') return strdup(target);
    return buildPath(expml_dir, target);
}

RunConfig* Storage_readConfig(const char* run_dir) {
    char* path = buildPath(run_dir, "config.json");
    char* content = readFileToString(path);
    free(path);
    if (!content) return NULL;

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json) return NULL;

    RunConfig* config = calloc(1, sizeof(RunConfig));
    config->json = json;
    return config;
}

void Storage_freeRunConfig(RunConfig* config) {
    if (!config) return;
    if (config->json) cJSON_Delete(config->json);
    free(config);
}

RunMetadata* Storage_readMetadata(const char* run_dir) {
    char* path = buildPath(run_dir, "metadata.json");
    char* content = readFileToString(path);
    free(path);
    if (!content) return NULL;

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json) return NULL;

    RunMetadata* meta = calloc(1, sizeof(RunMetadata));
    if (!meta) { cJSON_Delete(json); return NULL; }

    meta->run_id = getJsonString(json, "id", "unknown");
    meta->run_name = getJsonString(json, "name", "unknown");
    meta->user = getJsonString(json, "user", NULL);
    meta->host = getJsonString(json, "host", NULL);
    meta->os = getJsonString(json, "os", NULL);
    meta->python = getJsonString(json, "python", NULL);
    meta->gpu_name = getJsonString(json, "gpu_name", NULL);
    meta->disk_total = getJsonString(json, "disk_total", NULL);
    meta->ram_total = getJsonString(json, "ram_total", NULL);
    meta->command = getJsonString(json, "command", NULL);

    meta->cpu_count = getJsonInt(json, "cpu_count", 0);
    meta->gpu_count = getJsonInt(json, "gpu_count", 0);

    cJSON_Delete(json);
    return meta;
}

void Storage_freeRunMetadata(RunMetadata* meta) {
    if (!meta) return;
    free(meta->run_id);
    free(meta->run_name);
    free(meta->user);
    free(meta->host);
    free(meta->os);
    free(meta->python);
    free(meta->gpu_name);
    free(meta->disk_total);
    free(meta->ram_total);
    free(meta->command);
    free(meta);
}

RunSummary* Storage_readSummary(const char* run_dir) {
    char* path = buildPath(run_dir, "summary.json");
    char* content = readFileToString(path);
    free(path);
    if (!content) return NULL;

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json) return NULL;

    RunSummary* sum = calloc(1, sizeof(RunSummary));
    
    sum->status = getJsonString(json, "status", "UNKNOWN");
    sum->runtime = getJsonDouble(json, "_runtime", 0.0);
    sum->timestamp = getJsonDouble(json, "_timestamp", 0.0);
    sum->step = getJsonInt(json, "_step", 0);
    sum->epoch = getJsonInt(json, "epoch", 0);
    sum->json = json; 

    return sum;
}

void Storage_freeRunSummary(RunSummary* summary) {
    if (!summary) return;
    free(summary->status);
    if (summary->json) cJSON_Delete(summary->json);
    free(summary);
}

void* Storage_openMetrics(const char* run_dir) {
    char* path = buildPath(run_dir, "metrics.jsonl");
    FILE* f = fopen(path, "r");
    free(path);
    
    if (!f) return NULL;

    MetricsHandle* h = malloc(sizeof(MetricsHandle));
    h->file = f;
    h->line_buffer = NULL;
    h->buffer_size = 0;
    return h;
}

MetricEntry* Storage_readNextMetric(void* handle) {
    MetricsHandle* h = (MetricsHandle*)handle;
    if (!h) return NULL;

    ssize_t read = getline(&h->line_buffer, &h->buffer_size, h->file);
    if (read < 0) return NULL; 

    cJSON* json = cJSON_Parse(h->line_buffer);
    if (!json) return NULL; 

    MetricEntry* entry = calloc(1, sizeof(MetricEntry));
    entry->json = json;
    entry->step = getJsonInt(json, "_step", -1);
    entry->timestamp = getJsonDouble(json, "_timestamp", 0.0);

    return entry;
}

void Storage_closeMetrics(void* handle) {
    MetricsHandle* h = (MetricsHandle*)handle;
    if (!h) return;
    if (h->file) fclose(h->file);
    if (h->line_buffer) free(h->line_buffer);
    free(h);
}

void Storage_freeMetricEntry(MetricEntry* entry) {
    if (!entry) return;
    if (entry->json) cJSON_Delete(entry->json);
    free(entry);
}
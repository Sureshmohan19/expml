#define _POSIX_C_SOURCE 200809L 

#include "Storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <cjson/cJSON.h>

typedef struct MetricsHandle_ {
    FILE* file;
    char* line_buffer;
    size_t buffer_size;
} MetricsHandle;

// Reads entire file content into a dynamically allocated string
static char* readFileToString(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) { fclose(f); return NULL; }

    char* content = malloc((size_t)size + 1);
    if (!content) { fclose(f); return NULL; }
    
    // Read up to 'size' bytes to prevent overflow if file grew
    size_t read = fread(content, 1, (size_t)size, f);
    content[read] = '\0';
    fclose(f);
    return content;
}

// Helper: Extracts the folder name from a full path (e.g., "/tmp/run-123" -> "run-123")
static char* extractFolderName(const char* path) {
    if (!path) return strdup("unknown");
    
    // Create a copy because we might need to modify it (strip trailing slash)
    char* copy = strdup(path);
    size_t len = strlen(copy);
    
    // Remove trailing slash if present (e.g., "run-123/")
    if (len > 1 && copy[len-1] == '/') {
        copy[len-1] = '\0';
    }
    
    char* base = strrchr(copy, '/');
    char* result = strdup(base ? base + 1 : copy);
    
    free(copy);
    return result;
}

// Builds a file path by concatenating directory and filename with a separator
static char* buildPath(const char* dir, const char* filename) {
    size_t len = strlen(dir) + strlen(filename) + 2; 
    char* path = malloc(len);
    if (!path) return NULL;
    snprintf(path, len, "%s/%s", dir, filename);
    return path;
}

// Extracts a string value from JSON object, returns duplicate or default
static char* getJsonString(cJSON* root, const char* key, const char* def) {
    cJSON* item = cJSON_GetObjectItem(root, key);
    if (item && cJSON_IsString(item)) { return strdup(item->valuestring); }
    return def ? strdup(def) : NULL;
}

// Extracts an integer value from JSON object, returns value or default
static int getJsonInt(cJSON* root, const char* key, int def) {
    cJSON* item = cJSON_GetObjectItem(root, key);
    if (item && cJSON_IsNumber(item)) { return item->valueint; }
    return def;
}

// Extracts a double value from JSON object, returns value or default
static double getJsonDouble(cJSON* root, const char* key, double def) {
    cJSON* item = cJSON_GetObjectItem(root, key);
    if (item && cJSON_IsNumber(item)) { return item->valuedouble; }
    return def;
}

// Finds and returns the path to the most recent run directory in the given expml directory
char* Storage_findLatestRun(const char* expml_dir) {
    char* symlink_path = buildPath(expml_dir, "latest-run");
    if (!symlink_path) return NULL;

    struct stat st;
    if (lstat(symlink_path, &st) != 0 || !S_ISLNK(st.st_mode)) {
        free(symlink_path);
        return NULL;
    }
    
    // Use PATH_MAX for safer buffer size, fallback to 1024 if not defined
    #ifndef PATH_MAX
    #define PATH_MAX 1024
    #endif
    
    char target[PATH_MAX];
    ssize_t len = readlink(symlink_path, target, sizeof(target) - 1);
    
    if (len < 0) {
        free(symlink_path);
        return NULL;
    }
    
    target[len] = '\0';
    
    // If target is absolute path, return it directly; otherwise resolve relative to expml_dir
    char* result;
    if (target[0] == '/') {
        result = strdup(target);
    } else {
        result = buildPath(expml_dir, target);
    }
    
    free(symlink_path);
    return result;
}

// Reads and returns the configuration for a specific run
RunConfig* Storage_readConfig(const char* run_dir) {
    char* path = buildPath(run_dir, "config.json");
    if (!path) return NULL;
    
    char* content = readFileToString(path);
    free(path);
    if (!content) return NULL;

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json) return NULL;

    RunConfig* config = calloc(1, sizeof(RunConfig));
    if (!config) {
        cJSON_Delete(json);
        return NULL;
    }
    
    config->json = json;
    return config;
}

// Frees memory allocated for a RunConfig structure
void Storage_freeRunConfig(RunConfig* config) {
    if (!config) return;
    if (config->json) cJSON_Delete(config->json);
    free(config);
}

// Reads and returns the metadata for a specific run
RunMetadata* Storage_readMetadata(const char* run_dir) {
    char* path = buildPath(run_dir, "metadata.json");
    if (!path) return NULL;
    
    char* content = readFileToString(path);
    free(path);
    if (!content) return NULL;

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json) return NULL;

    RunMetadata* meta = calloc(1, sizeof(RunMetadata));
    if (!meta) { cJSON_Delete(json); return NULL; }

    // Extract all string fields with defaults
    meta->run_id = getJsonString(json, "id", "unknown");
    
    // LOGIC CHANGE: If "name" is missing in JSON, use the directory name (e.g. "run-2025...")
    char* folder_name = extractFolderName(run_dir);
    meta->run_name = getJsonString(json, "name", folder_name); 
    free(folder_name);

    meta->user = getJsonString(json, "user", NULL);
    meta->host = getJsonString(json, "host", NULL);
    meta->os = getJsonString(json, "os", NULL);
    meta->python = getJsonString(json, "python", NULL);
    meta->gpu_name = getJsonString(json, "gpu_name", NULL);
    meta->disk_total = getJsonString(json, "disk_total", NULL);
    meta->ram_total = getJsonString(json, "ram_total", NULL);
    meta->command = getJsonString(json, "command", NULL);

    // Check if any required allocation failed (run_id and run_name have defaults)
    if (!meta->run_id || !meta->run_name) {
        cJSON_Delete(json);
        Storage_freeRunMetadata(meta);
        return NULL;
    }

    // Extract integer fields
    meta->cpu_count = getJsonInt(json, "cpu_count", 0);
    meta->gpu_count = getJsonInt(json, "gpu_count", 0);

    cJSON_Delete(json);
    return meta;
}

// Frees memory allocated for a RunMetadata structure
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

// Reads and returns the summary information for a specific run
RunSummary* Storage_readSummary(const char* run_dir) {
    char* path = buildPath(run_dir, "summary.json");
    if (!path) return NULL;
    
    char* content = readFileToString(path);
    free(path);
    if (!content) return NULL;

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json) return NULL;

    RunSummary* sum = calloc(1, sizeof(RunSummary));
    if (!sum) {
        cJSON_Delete(json);
        return NULL;
    }
    
    // Extract summary fields
    sum->status = getJsonString(json, "status", "UNKNOWN");
    
    // Check if required allocation failed
    if (!sum->status) {
        cJSON_Delete(json);
        free(sum);
        return NULL;
    }
    
    sum->runtime = getJsonDouble(json, "_runtime", 0.0);
    sum->timestamp = getJsonDouble(json, "_timestamp", 0.0);
    sum->step = getJsonInt(json, "_step", 0);
    sum->epoch = getJsonInt(json, "epoch", 0);
    sum->json = json;  // Keep JSON for additional fields

    return sum;
}

// Frees memory allocated for a RunSummary structure
void Storage_freeRunSummary(RunSummary* summary) {
    if (!summary) return;
    free(summary->status);
    if (summary->json) cJSON_Delete(summary->json);
    free(summary);
}

// Opens the metrics file for sequential reading and returns an opaque handle
void* Storage_openMetrics(const char* run_dir) {
    char* path = buildPath(run_dir, "metrics.jsonl");
    if (!path) return NULL;
    
    FILE* f = fopen(path, "r");
    free(path);
    
    if (!f) return NULL;

    MetricsHandle* h = malloc(sizeof(MetricsHandle));
    if (!h) {
        fclose(f);
        return NULL;
    }
    
    h->file = f;
    h->line_buffer = NULL;
    h->buffer_size = 0;
    return h;
}

// Reads the next metric entry from an open metrics handle, or NULL if no more entries
MetricEntry* Storage_readNextMetric(void* handle) {
    MetricsHandle* h = (MetricsHandle*)handle;
    if (!h) return NULL;

    // Read next line from file (getline allocates/reallocs buffer automatically)
    ssize_t read = getline(&h->line_buffer, &h->buffer_size, h->file);
    if (read < 0) return NULL;  // EOF or error

    cJSON* json = cJSON_Parse(h->line_buffer);
    if (!json) return NULL;  // Invalid JSON line

    MetricEntry* entry = calloc(1, sizeof(MetricEntry));
    if (!entry) {
        cJSON_Delete(json);
        return NULL;
    }
    
    entry->json = json;
    entry->step = getJsonInt(json, "_step", -1);
    entry->timestamp = getJsonDouble(json, "_timestamp", 0.0);

    return entry;
}

// Closes an open metrics handle and releases associated resources
void Storage_closeMetrics(void* handle) {
    MetricsHandle* h = (MetricsHandle*)handle;
    if (!h) return;
    if (h->file) fclose(h->file);
    if (h->line_buffer) free(h->line_buffer);
    free(h);
}

// Frees memory allocated for a MetricEntry structure
void Storage_freeMetricEntry(MetricEntry* entry) {
    if (!entry) return;
    if (entry->json) cJSON_Delete(entry->json);
    free(entry);
}
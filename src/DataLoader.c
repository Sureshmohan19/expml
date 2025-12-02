#include "Storage.h"
#include "DataLoader.h"
#include "SystemPanel.h"
#include "MetricsPanel.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#define METRIC_INITIAL_CAPACITY 1024

typedef struct {
    char* key;
    float* values;
    int count;
    int capacity;
} MetricSeries;

// Finds existing series by key or creates a new one in the list
static MetricSeries* getSeries(MetricSeries** list, int* count, const char* key) {
    // Look for existing series with matching key
    for (int i = 0; i < *count; i++) {
        if (strcmp((*list)[i].key, key) == 0) { return &(*list)[i]; }
    }
    
    // Expand the series list to accommodate new series
    MetricSeries* new_list = realloc(*list, (*count + 1) * sizeof(MetricSeries));
    if (!new_list) return NULL; 
    
    *list = new_list;
    MetricSeries* s = &(*list)[*count];
    
    // Initialize new series with duplicated key
    s->key = strdup(key);
    if (!s->key) {
        return NULL;
    }
    
    s->count = 0;
    s->capacity = METRIC_INITIAL_CAPACITY;
    s->values = malloc(s->capacity * sizeof(float));
    
    // Clean up key if values allocation fails
    if (!s->values) {
        free(s->key);
        s->key = NULL;  // Mark as invalid
        return NULL;
    }
    
    // Only increment count after successful allocation
    (*count)++;
    return s;
}

// Appends a value to a metric series, expanding capacity if needed
static void appendValue(MetricSeries* s, float val) {
    if (!s || !s->values) return;
    
    // Double capacity when full
    if (s->count >= s->capacity) {
        s->capacity *= 2;
        float* new_vals = realloc(s->values, s->capacity * sizeof(float));
        if (!new_vals) {
            // Silently drop value on allocation failure - series remains valid
            return;
        }
        s->values = new_vals;
    }
    s->values[s->count++] = val;
}

// Frees all memory associated with a list of metric series
static void freeAllSeries(MetricSeries* list, int count) {
    if (!list) return;
    for (int i = 0; i < count; i++) {
        free(list[i].key);
        free(list[i].values);
    }
    free(list);
}

// Loads metrics and system data from a run directory and populates the corresponding panels
void DataLoader_loadMetrics(const char* run_path, Panel* metricsPanel, Panel* systemPanel) {
    void* handle = Storage_openMetrics(run_path);
    if (!handle) return;

    MetricSeries* all_series = NULL;
    int series_count = 0;

    // Read all metric entries and organize into series
    MetricEntry* entry;
    while ((entry = Storage_readNextMetric(handle)) != NULL) {
        if (entry->json) {
            cJSON* item;
            // Iterate through all fields in the JSON object
            cJSON_ArrayForEach(item, entry->json) {
                // Skip internal fields (prefixed with '_') and non-numeric values
                if (item->string[0] == '_') continue;
                if (!cJSON_IsNumber(item)) continue;

                // Get or create series for this metric key
                MetricSeries* s = getSeries(&all_series, &series_count, item->string);
                if (s) {
                    appendValue(s, (float)item->valuedouble);
                }
            }
        }
        Storage_freeMetricEntry(entry);
    }
    Storage_closeMetrics(handle);

    // Clear existing panel contents
    if (metricsPanel) Panel_clear(metricsPanel);
    if (systemPanel) Panel_clear(systemPanel);

    // Populate panels with collected metrics
    for (int i = 0; i < series_count; i++) {
        MetricSeries* s = &all_series[i];
        
        // Skip empty or invalid series
        if (s->count == 0 || !s->key || !s->values) continue;

        // Get the most recent value from the series
        float current = s->values[s->count - 1];

        // Route system metrics to system panel with appropriate formatting
        if (strncmp(s->key, "system/", 7) == 0) {
            if (systemPanel) {
                char* display_name = s->key + 7;  // Strip "system/" prefix
                char val_str[64];
                
                // Format value based on metric type heuristics
                if (strstr(display_name, "percent") || strstr(display_name, "util") || strstr(display_name, "load")) {
                     snprintf(val_str, sizeof(val_str), "%.1f%%", current);
                } else if (strstr(display_name, "gb") || strstr(display_name, "ram")) {
                     snprintf(val_str, sizeof(val_str), "%.2fGB", current);
                } else if (strstr(display_name, "temp")) {
                     snprintf(val_str, sizeof(val_str), "%.0f°C", current);
                } else {
                     snprintf(val_str, sizeof(val_str), "%.4f", current);
                }

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s\t%s", display_name, val_str);
                Panel_addItem(systemPanel, buffer, NULL);
            }
        } else {
            // Route regular metrics to metrics panel with full time series data
            if (metricsPanel) {
                MetricsPanel_addMetric(metricsPanel, s->key, current, s->values, s->count);
            }
        }
    }
    
    // Clean up all allocated series data
    freeAllSeries(all_series, series_count);
}
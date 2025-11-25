#include "DataLoader.h"
#include "Storage.h"
#include "MetricsPanel.h"
#include "SystemPanel.h"

#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// --- Internal Helper Structs ---

typedef struct {
    char* key;
    float* values;
    int count;
    int capacity;
} MetricSeries;

// --- Internal Helper Functions ---

static MetricSeries* getSeries(MetricSeries** list, int* count, const char* key) {
    for (int i = 0; i < *count; i++) {
        if (strcmp((*list)[i].key, key) == 0) {
            return &(*list)[i];
        }
    }
    
    // Not found, create new
    MetricSeries* new_list = realloc(*list, (*count + 1) * sizeof(MetricSeries));
    if (!new_list) return NULL; // Safety check
    
    *list = new_list;
    MetricSeries* s = &(*list)[*count];
    
    s->key = strdup(key);
    s->count = 0;
    s->capacity = 1024;
    s->values = malloc(s->capacity * sizeof(float));
    
    (*count)++;
    return s;
}

static void appendValue(MetricSeries* s, float val) {
    if (!s || !s->values) return;
    
    if (s->count >= s->capacity) {
        s->capacity *= 2;
        float* new_vals = realloc(s->values, s->capacity * sizeof(float));
        if (!new_vals) return;
        s->values = new_vals;
    }
    s->values[s->count++] = val;
}

// --- Public API Implementation ---

void DataLoader_loadMetrics(const char* run_path, Panel* metricsPanel, Panel* systemPanel) {
    void* handle = Storage_openMetrics(run_path);
    if (!handle) return;

    // Temporary storage for accumulating time-series data
    MetricSeries* all_series = NULL;
    int series_count = 0;

    // 1. READ Loop
    MetricEntry* entry;
    while ((entry = Storage_readNextMetric(handle)) != NULL) {
        if (entry->json) {
            cJSON* item;
            cJSON_ArrayForEach(item, entry->json) {
                // Filter out internal keys and non-numbers
                if (item->string[0] == '_') continue;
                if (!cJSON_IsNumber(item)) continue;

                MetricSeries* s = getSeries(&all_series, &series_count, item->string);
                if (s) {
                    appendValue(s, (float)item->valuedouble);
                }
            }
        }
        Storage_freeMetricEntry(entry);
    }
    Storage_closeMetrics(handle);

    // 2. POPULATE Panels
    // Clear existing data
    if (metricsPanel) Panel_clear(metricsPanel);
    if (systemPanel) Panel_clear(systemPanel);

    for (int i = 0; i < series_count; i++) {
        MetricSeries* s = &all_series[i];
        if (s->count == 0) {
            free(s->key);
            free(s->values);
            continue;
        }

        float current = s->values[s->count - 1];

        // Separation Logic: System vs Training Metrics
        if (strncmp(s->key, "system/", 7) == 0) {
            // -- RIGHT PANEL (System) --
            if (systemPanel) {
                char* display_name = s->key + 7; // Skip "system/" prefix
                char val_str[32];
                
                // Simple auto-formatting
                if (strstr(display_name, "percent") || strstr(display_name, "util") || strstr(display_name, "load")) {
                     snprintf(val_str, 32, "%.1f%%", current);
                } else if (strstr(display_name, "gb") || strstr(display_name, "ram")) {
                     snprintf(val_str, 32, "%.1fGB", current);
                } else if (strstr(display_name, "temp")) {
                     snprintf(val_str, 32, "%.1f°C", current);
                } else {
                     snprintf(val_str, 32, "%.2f", current);
                }

                char buffer[128];
                snprintf(buffer, sizeof(buffer), "%s\t%s", display_name, val_str);
                Panel_addItem(systemPanel, buffer, NULL);
            }
        } else {
            // -- MIDDLE PANEL (Training Charts) --
            if (metricsPanel) {
                MetricsPanel_addMetric(metricsPanel, s->key, current, s->values, s->count);
            }
        }

        // Cleanup individual series
        free(s->key);
        free(s->values);
    }
    
    // Cleanup list
    free(all_series);
}

#include "Storage.h"
#include "DataLoader.h"
#include "SystemPanel.h"
#include "MetricsPanel.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

typedef struct {
    char* key;
    float* values;
    int count;
    int capacity;
} MetricSeries;

static MetricSeries* getSeries(MetricSeries** list, int* count, const char* key) {
    for (int i = 0; i < *count; i++) {
        if (strcmp((*list)[i].key, key) == 0) { return &(*list)[i]; }
    }
    
    MetricSeries* new_list = realloc(*list, (*count + 1) * sizeof(MetricSeries));
    if (!new_list) return NULL; 
    
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

void DataLoader_loadMetrics(const char* run_path, Panel* metricsPanel, Panel* systemPanel) {
    void* handle = Storage_openMetrics(run_path);
    if (!handle) return;

    MetricSeries* all_series = NULL;
    int series_count = 0;

    MetricEntry* entry;
    while ((entry = Storage_readNextMetric(handle)) != NULL) {
        if (entry->json) {
            cJSON* item;
            cJSON_ArrayForEach(item, entry->json) {
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

        if (strncmp(s->key, "system/", 7) == 0) {
            if (systemPanel) {
                char* display_name = s->key + 7; 
                char val_str[32];
                
                if (strstr(display_name, "percent") || strstr(display_name, "util") || strstr(display_name, "load")) {
                     snprintf(val_str, 32, "%.1f%%", current);
                } else if (strstr(display_name, "gb") || strstr(display_name, "ram")) {
                     snprintf(val_str, 32, "%.2fGB", current);
                } else if (strstr(display_name, "temp")) {
                     snprintf(val_str, 32, "%.0f°C", current);
                } else {
                     snprintf(val_str, 32, "%.4f", current);
                }

                char buffer[128];
                snprintf(buffer, sizeof(buffer), "%s\t%s", display_name, val_str);
                Panel_addItem(systemPanel, buffer, NULL);
            }
        } else {
            if (metricsPanel) {
                MetricsPanel_addMetric(metricsPanel, s->key, current, s->values, s->count);
            }
        }
        free(s->key);
        free(s->values);
    }
    free(all_series);
}
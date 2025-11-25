#include "Storage.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>

void print_separator() {
    printf("----------------------------------------\n");
}

int main() {
    printf("=== Testing Storage Layer ===\n\n");

    // 1. Test findLatestRun
    char* latest = Storage_findLatestRun("expml");
    if (latest) {
        printf("✅ Latest Run Found: %s\n", latest);
    } else {
        printf("❌ Failed to find latest run\n");
        return 1;
    }
    print_separator();

    // 2. Test Metadata
    RunMetadata* meta = Storage_readMetadata(latest);
    if (meta) {
        printf("✅ Metadata Read:\n");
        printf("   ID:   %s\n", meta->run_id);
        printf("   Host: %s\n", meta->host);
        printf("   GPU:  %s\n", meta->gpu_name);
        Storage_freeRunMetadata(meta);
    } else {
        printf("❌ Failed to read metadata\n");
    }
    print_separator();

    // 3. Test Config
    RunConfig* config = Storage_readConfig(latest);
    if (config) {
        printf("✅ Config Read:\n");
        cJSON* lr = cJSON_GetObjectItem(config->json, "learning_rate");
        cJSON* model = cJSON_GetObjectItem(config->json, "model_type");
        if (lr) printf("   Learning Rate: %f\n", lr->valuedouble);
        if (model) printf("   Model Type:    %s\n", model->valuestring);
        Storage_freeRunConfig(config);
    } else {
        printf("❌ Failed to read config\n");
    }
    print_separator();

    // 4. Test Summary
    RunSummary* sum = Storage_readSummary(latest);
    if (sum) {
        printf("✅ Summary Read:\n");
        printf("   Status:  %s\n", sum->status);
        printf("   Runtime: %.2fs\n", sum->runtime);
        
        cJSON* loss = cJSON_GetObjectItem(sum->json, "loss");
        if (loss) printf("   Final Loss: %.3f\n", loss->valuedouble);
        
        Storage_freeRunSummary(sum);
    } else {
        printf("❌ Failed to read summary\n");
    }
    print_separator();

    // 5. Test Metrics Stream
    printf("✅ Metrics Stream:\n");
    void* handle = Storage_openMetrics(latest);
    if (handle) {
        MetricEntry* entry;
        while ((entry = Storage_readNextMetric(handle)) != NULL) {
            cJSON* loss = cJSON_GetObjectItem(entry->json, "loss");
            cJSON* cpu = cJSON_GetObjectItem(entry->json, "system/cpu");
            
            printf("   Step %d: Loss=%.2f, CPU=%.1f%%\n", 
                   entry->step, 
                   loss ? loss->valuedouble : -1.0,
                   cpu ? cpu->valuedouble : -1.0);
            
            Storage_freeMetricEntry(entry);
        }
        Storage_closeMetrics(handle);
    } else {
        printf("❌ Failed to open metrics.jsonl\n");
    }
    print_separator();

    free(latest);
    printf("=== Test Complete ===\n");
    return 0;
}

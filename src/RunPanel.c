#include "RunPanel.h"
#include "Terminal.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

#define VALUE_COLUMN_OFFSET 20

// --- Helper: Draw Item (Same as before) ---
static void RunPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool selected) {
    PanelItem* item = Panel_getItem(panel, index);
    if (!item) return;

    char* text = item->text;
    char* separator = strchr(text, '\t');

    // Clear line
    if (selected) attron(Terminal_colors[TEXT_SELECTED]);
    else attron(Terminal_colors[TEXT_NORMAL]);
    mvhline(y, x, ' ', w);

    if (separator) {
        // "Key\tValue"
        int key_len = separator - text;
        mvprintw(y, x, "%.*s", key_len, text);

        int val_x = x + VALUE_COLUMN_OFFSET;
        if (val_x < x + key_len + 1) val_x = x + key_len + 1;

        if (!selected) attron(Terminal_colors[TEXT_BRIGHT]);
        mvprintw(y, val_x, "%.*s", w - (val_x - x), separator + 1);
        if (!selected) attroff(Terminal_colors[TEXT_BRIGHT]);
    } else {
        // Header
        if (strlen(text) > 0) {
            if (!selected) attron(Terminal_colors[PANEL_HEADER]);
            attron(A_BOLD);
            mvprintw(y, x, "%.*s", w, text);
            attroff(A_BOLD);
            if (!selected) attroff(Terminal_colors[PANEL_HEADER]);
        }
    }

    if (selected) attroff(Terminal_colors[TEXT_SELECTED]);
    else attroff(Terminal_colors[TEXT_NORMAL]);
}

Panel* RunPanel_new(int x, int y, int w, int h) {
    Panel* p = Panel_new(x, y, w, h, "Run Overview");
    if (!p) return NULL;
    Panel_setDrawItem(p, RunPanel_drawItem);
    return p;
}

// --- Helper: Add Key-Value string ---
static void addKV(Panel* p, const char* key, const char* val) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\t%s", key, val ? val : "N/A");
    Panel_addItem(p, buffer, NULL);
}

// --- Helper: Add Key-Int string ---
static void addKI(Panel* p, const char* key, int val) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\t%d", key, val);
    Panel_addItem(p, buffer, NULL);
}

void RunPanel_setData(Panel* this, RunConfig* config, RunMetadata* meta, RunSummary* summary) {
    if (!this) return;

    // Clear existing items to rebuild the list
    Panel_clear(this);

    // 1. Summary Section (Top Priority)
    if (summary) {
        Panel_addItem(this, "Summary", NULL);
        addKV(this, "State", summary->status);
        
        char rt[32];
        snprintf(rt, 32, "%.1fs", summary->runtime);
        addKV(this, "Runtime", rt);
        addKI(this, "Step", summary->step);
        addKI(this, "Epoch", summary->epoch);

        // Add dynamic summary metrics (loss, acc, etc.)
        if (summary->json) {
            cJSON* item;
            cJSON_ArrayForEach(item, summary->json) {
                // Skip internal keys starting with _ or status/epoch which we already showed
                if (item->string[0] == '_' || 
                    strcmp(item->string, "status") == 0 || 
                    strcmp(item->string, "epoch") == 0) continue;

                if (cJSON_IsNumber(item)) {
                    char valStr[32];
                    snprintf(valStr, 32, "%.4f", item->valuedouble);
                    addKV(this, item->string, valStr);
                }
            }
        }
        Panel_addItem(this, "", NULL); // Spacer
    }

    // 2. Metadata Section
    if (meta) {
        Panel_addItem(this, "Environment", NULL);
        addKV(this, "ID", meta->run_id);
        addKV(this, "Name", meta->run_name);
        addKV(this, "Host", meta->host);
        addKV(this, "User", meta->user);
        addKV(this, "OS", meta->os);
        addKV(this, "Python", meta->python);
        addKV(this, "GPU", meta->gpu_name);
        addKI(this, "CPUs", meta->cpu_count);
        addKI(this, "GPUs", meta->gpu_count);
        Panel_addItem(this, "", NULL);
    }

    // 3. Config Section
    if (config && config->json) {
        Panel_addItem(this, "Config", NULL);
        
        cJSON* item;
        cJSON_ArrayForEach(item, config->json) {
            char valBuffer[64];
            
            if (cJSON_IsString(item)) {
                addKV(this, item->string, item->valuestring);
            } else if (cJSON_IsNumber(item)) {
                // Formatting based on whether it's int or float
                if ((double)item->valueint == item->valuedouble) {
                     snprintf(valBuffer, 64, "%d", item->valueint);
                } else {
                     snprintf(valBuffer, 64, "%.4f", item->valuedouble);
                }
                addKV(this, item->string, valBuffer);
            } else if (cJSON_IsBool(item)) {
                addKV(this, item->string, cJSON_IsTrue(item) ? "true" : "false");
            }
        }
    }
}

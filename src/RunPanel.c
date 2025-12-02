#include "RunPanel.h"
#include "Terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <cjson/cJSON.h>

#define VALUE_COLUMN_OFFSET 15  // Reduced from 20 to 15

static void RunPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool selected) {
    PanelItem* item = Panel_getItem(panel, index);
    if (!item) return;
    char* text = item->text;
    char* separator = strchr(text, '\t');

    if (selected) attron(Terminal_colors[TEXT_SELECTED]);
    else attron(Terminal_colors[TEXT_NORMAL]);
    mvhline(y, x, ' ', w);

    if (separator) {
        int key_len = separator - text;
        int max_key_len = VALUE_COLUMN_OFFSET - 1;  // Leave 1 space before value
        
        // Draw key with TEXT_DIM
        if (!selected) attron(Terminal_colors[TEXT_DIM]);
        if (key_len > max_key_len) {
            mvprintw(y, x, "%.*s...", max_key_len - 3, text);
        } else {
            mvprintw(y, x, "%.*s", key_len, text);
        }
        if (!selected) attroff(Terminal_colors[TEXT_DIM]);

        // Draw value
        int val_x = x + VALUE_COLUMN_OFFSET;
        int available_width = w - VALUE_COLUMN_OFFSET - 1;  // -1 for safety margin
        const char* value = separator + 1;
        int value_len = strlen(value);
        
        if (!selected) attron(Terminal_colors[TEXT_BRIGHT]);
        if (value_len > available_width) {
            mvprintw(y, val_x, "%.*s...", available_width - 3, value);
        } else {
            mvprintw(y, val_x, "%.*s", available_width, value);
        }
        if (!selected) attroff(Terminal_colors[TEXT_BRIGHT]);
    } else {
        // Section headers
        if (strlen(text) > 0) {
            if (!selected) attron(Terminal_colors[PANEL_HEADER]);
            attron(A_BOLD);
            if (strlen(text) > (size_t)(w - 1)) {
                mvprintw(y, x, "%.*s...", w - 4, text);
            } else {
                mvprintw(y, x, "%.*s", w - 1, text);
            }
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

static void addKV(Panel* p, const char* key, const char* val) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\t%s", key, val ? val : "N/A");
    Panel_addItem(p, buffer, NULL);
}

static void addKI(Panel* p, const char* key, int val) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\t%d", key, val);
    Panel_addItem(p, buffer, NULL);
}

void RunPanel_setData(Panel* this, RunConfig* config, RunMetadata* meta, RunSummary* summary) {
    if (!this) return;
    Panel_clear(this);

    // Top 4 items (no heading)
    char state_val[32] = "N/A";
    char name_val[128] = "N/A";
    char id_val[128] = "N/A";
    
    if (summary && summary->status) {
        snprintf(state_val, sizeof(state_val), "%s", summary->status);
    }
    if (meta && meta->run_name) {
        snprintf(name_val, sizeof(name_val), "%s", meta->run_name);
    }
    if (meta && meta->run_id) {
        snprintf(id_val, sizeof(id_val), "%s", meta->run_id);
    }
    
    addKV(this, "State", state_val);
    addKV(this, "Name", name_val);
    addKV(this, "Project", "N/A");
    addKV(this, "ID", id_val);
    Panel_addItem(this, "", NULL);

    // Environment section
    if (meta) {
        Panel_addItem(this, "Environment", NULL);
        addKV(this, "Host", meta->host);
        addKV(this, "User", meta->user);
        addKV(this, "OS", meta->os);
        addKV(this, "Python", meta->python);
        addKV(this, "GPU", meta->gpu_name);
        addKI(this, "CPUs", meta->cpu_count);
        addKI(this, "GPUs", meta->gpu_count);
        addKV(this, "Disk", meta->disk_total);
        addKV(this, "RAM", meta->ram_total);
        addKV(this, "Command", meta->command);
        Panel_addItem(this, "", NULL);
    }

    // Configuration section
    if (config && config->json) {
        Panel_addItem(this, "Configuration", NULL);
        
        cJSON* item;
        cJSON_ArrayForEach(item, config->json) {
            char valBuffer[64];
            
            if (cJSON_IsString(item)) {
                addKV(this, item->string, item->valuestring);
            } else if (cJSON_IsNumber(item)) {
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
        Panel_addItem(this, "", NULL);
    }

    // Summary section
    if (summary) {
        Panel_addItem(this, "Summary", NULL);
        
        addKV(this, "status", summary->status);
        
        char rt[32];
        snprintf(rt, 32, "%.1fs", summary->runtime);
        addKV(this, "_runtime", rt);
        
        char ts[32];
        snprintf(ts, 32, "%.2f", summary->timestamp);
        addKV(this, "_timestamp", ts);
        
        addKI(this, "_step", summary->step);
        addKI(this, "epoch", summary->epoch);

        // Add any other fields from summary.json
        if (summary->json) {
            cJSON* item;
            cJSON_ArrayForEach(item, summary->json) {
                // Skip fields we already added
                if (strcmp(item->string, "status") == 0 || 
                    strcmp(item->string, "_runtime") == 0 ||
                    strcmp(item->string, "_timestamp") == 0 ||
                    strcmp(item->string, "_step") == 0 ||
                    strcmp(item->string, "epoch") == 0) continue;

                if (cJSON_IsNumber(item)) {
                    char valStr[32];
                    snprintf(valStr, 32, "%.4f", item->valuedouble);
                    addKV(this, item->string, valStr);
                }
            }
        }
    }
}
#include "SystemPanel.h"
#include "Terminal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

// Fixed column position for values.
// System keys (e.g. "gpu_utilization") are long, so we need more space than RunPanel.
#define VALUE_COLUMN_OFFSET 22

static void SystemPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool selected) {
    PanelItem* item = Panel_getItem(panel, index);
    if (!item) return;
    char* text = item->text;
    char* separator = strchr(text, '\t');

    // 1. Draw Background (Full Width)
    if (selected) attron(Terminal_colors[TEXT_SELECTED]);
    else attron(Terminal_colors[TEXT_NORMAL]);
    mvhline(y, x, ' ', w);

    if (separator) {
        // Calculate lengths
        int key_len = separator - text;
        int max_key_len = VALUE_COLUMN_OFFSET - 1; // Leave 1 char gap
        
        // --- DRAW KEY (Left Column) ---
        if (!selected) attron(Terminal_colors[TEXT_DIM]);
        
        if (key_len > max_key_len) {
            // Truncate with "..."
            mvprintw(y, x, "%.*s...", max_key_len - 3, text);
        } else {
            mvprintw(y, x, "%.*s", key_len, text);
        }
        
        if (!selected) attroff(Terminal_colors[TEXT_DIM]);

        // --- DRAW VALUE (Fixed Right Column) ---
        int val_x = x + VALUE_COLUMN_OFFSET;
        int available_width = w - VALUE_COLUMN_OFFSET - 1; // -1 for padding
        char* val = separator + 1;
        int val_len = strlen(val);

        if (!selected) attron(Terminal_colors[TEXT_BRIGHT]);
        attron(A_BOLD); // Keep values bold for visibility

        if (val_len > available_width) {
            // Truncate with "..."
            mvprintw(y, val_x, "%.*s...", available_width - 3, val);
        } else {
            mvprintw(y, val_x, "%s", val);
        }

        attroff(A_BOLD);
        if (!selected) attroff(Terminal_colors[TEXT_BRIGHT]);

    } else {
        // Fallback for non-KV lines (rare in SystemPanel)
        if (!selected) attron(Terminal_colors[TEXT_DIM]);
        mvprintw(y, x, "%.*s", w, text);
        if (!selected) attroff(Terminal_colors[TEXT_DIM]);
    }

    // Reset Selection Attributes
    if (selected) attroff(Terminal_colors[TEXT_SELECTED]);
    else attroff(Terminal_colors[TEXT_NORMAL]);
}

Panel* SystemPanel_new(int x, int y, int w, int h) {
    Panel* p = Panel_new(x, y, w, h, "System Metrics");
    if (!p) return NULL;

    Panel_setDrawItem(p, SystemPanel_drawItem);
    return p;
}
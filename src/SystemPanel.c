#include "SystemPanel.h"
#include "Terminal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

#define DEFAULT_OFFSET 22

static void SystemPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool selected) {
    PanelItem* item = Panel_getItem(panel, index);
    if (!item) return;
    char* text = item->text;
    char* separator = strchr(text, '\t');

    // --- 1. DYNAMIC LAYOUT CALCULATION ---
    // If width is tight (< 32), reserve just 8 chars for the value (e.g., " 99.9% ")
    // Otherwise, use the standard indentation.
    int val_x_offset = (w < 32) ? (w - 8) : DEFAULT_OFFSET;
    
    // Safety clamp
    if (val_x_offset < 2) val_x_offset = 2;

    // --- 2. DRAW BACKGROUND ---
    if (selected) attron(Terminal_colors[TEXT_SELECTED]);
    else attron(Terminal_colors[TEXT_NORMAL]);
    mvhline(y, x, ' ', w);

    // --- 3. DRAW TEXT ---
    if (separator) {
        // Calculate lengths based on dynamic offset
        int key_len = separator - text;
        int max_key_len = val_x_offset - 1; // Leave 1 char gap
        
        // --- DRAW KEY (Left Column) ---
        if (!selected) attron(Terminal_colors[TEXT_DIM]);
        
        if (key_len > max_key_len) {
            // Truncate with ".." if key is too long for the space
            mvprintw(y, x, "%.*s..", (max_key_len > 2 ? max_key_len - 2 : 0), text);
        } else {
            mvprintw(y, x, "%.*s", key_len, text);
        }
        
        if (!selected) attroff(Terminal_colors[TEXT_DIM]);

        // --- DRAW VALUE (Right Column) ---
        int abs_val_x = x + val_x_offset;
        int available_width = w - val_x_offset; 
        char* val = separator + 1;
        int val_len = strlen(val);

        if (!selected) attron(Terminal_colors[TEXT_BRIGHT]);
        attron(A_BOLD); 

        if (val_len > available_width) {
            // Truncate value if it overflows
            mvprintw(y, abs_val_x, "%.*s..", (available_width > 2 ? available_width - 2 : 0), val);
        } else {
            mvprintw(y, abs_val_x, "%s", val);
        }

        attroff(A_BOLD);
        if (!selected) attroff(Terminal_colors[TEXT_BRIGHT]);

    } else {
        // Fallback for non-KV lines
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
    // Height defaults to 1 automatically in Panel_new
    
    return p;
}
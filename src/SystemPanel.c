#include "SystemPanel.h"
#include "Terminal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

// Reuse the offset logic, but smaller for the right panel
#define SYS_VALUE_OFFSET 20

static void SystemPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool selected) {
    PanelItem* item = Panel_getItem(panel, index);
    if (!item) return;

    char* text = item->text;
    char* separator = strchr(text, '\t');

    if (selected) {
        attron(Terminal_colors[TEXT_SELECTED]);
        mvhline(y, x, ' ', w);
    } else {
        attron(Terminal_colors[TEXT_NORMAL]);
        mvhline(y, x, ' ', w);
    }

    if (separator) {
        // Format: "Label\tValue"
        int key_len = separator - text;
        
        // Draw Label
        mvprintw(y, x, "%.*s", key_len, text);
        
        // Draw Value (Right aligned or Fixed Offset)
        // For System panel, let's align values to the right side of the available space
        // because the numbers are short (75%, 58.4W)
        
        attron(A_BOLD);
        if (!selected) attron(Terminal_colors[TEXT_BRIGHT]);
        
        char* val = separator + 1;
        int val_len = strlen(val);
        int val_pos = x + w - val_len - 1; // Align right (-1 for padding)
        
        if (val_pos < x + key_len + 1) val_pos = x + key_len + 1; // Safety overlap
        
        mvprintw(y, val_pos, "%s", val);
        
        if (!selected) attroff(Terminal_colors[TEXT_BRIGHT]);
        attroff(A_BOLD);
        
    } else {
        // Header or Spacer
        if (strlen(text) > 0) {
            if (!selected) attron(Terminal_colors[PANEL_HEADER]);
            mvprintw(y, x, "%s", text);
            if (!selected) attroff(Terminal_colors[PANEL_HEADER]);
        }
    }

    if (selected) attroff(Terminal_colors[TEXT_SELECTED]);
    else attroff(Terminal_colors[TEXT_NORMAL]);
}

Panel* SystemPanel_new(int x, int y, int w, int h) {
    Panel* p = Panel_new(x, y, w, h, "System Metrics [1-12 of 25]");
    if (!p) return NULL;

    Panel_setDrawItem(p, SystemPanel_drawItem);

    // Populate Data based on Screenshot
    Panel_addItem(p, "Apple E-cores\t75%", NULL);
    Panel_addItem(p, "Apple E-cores Freq\t2.03GHz", NULL);
    Panel_addItem(p, "Apple P-cores\t75%", NULL);
    Panel_addItem(p, "Apple P-cores Freq\t3.15GHz", NULL);
    Panel_addItem(p, "", NULL); // Spacer
    
    Panel_addItem(p, "CPU Power\t58.4W", NULL);
    Panel_addItem(p, "CPU Temp\t70°C", NULL);
    Panel_addItem(p, "", NULL);
    
    Panel_addItem(p, "Disk (%)\t75%", NULL);
    Panel_addItem(p, "Disk (GB)\t759.6GiB", NULL);
    Panel_addItem(p, "Disk I/O\t217.1MiB", NULL);
    Panel_addItem(p, "", NULL);

    Panel_addItem(p, "GPU Freq\t763MHz", NULL);
    Panel_addItem(p, "GPU Power\t18.3W", NULL);
    Panel_addItem(p, "GPU Temp\t59.6°C", NULL);

    return p;
}

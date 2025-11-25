#include "MetricsPanel.h"
#include "SparkLine.h"
#include "Terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define ITEM_HEIGHT 4

typedef struct {
    char* name;
    float current_value;
    float min_value;
    float max_value;
    float* history;
    int history_count;
} MetricData;

static void MetricsPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool selected) {
    PanelItem* item = Panel_getItem(panel, index);
    if (!item || !item->data) return;

    MetricData* m = (MetricData*)item->data;

    // --- Layout Calculations ---
    int text_width = 20; // Width for text info on left
    int chart_x = x + text_width + 2;
    int chart_w = w - text_width - 3;

    // Background Cleaning
    int bg_color = selected ? Terminal_colors[TEXT_SELECTED] : Terminal_colors[TEXT_NORMAL];
    
    // Fill the background for all 4 lines
    attron(bg_color);
    for(int i=0; i<ITEM_HEIGHT; i++) {
        mvhline(y + i, x, ' ', w);
        
        // Draw Separator Line (Vertical) between text and chart
        if (i < ITEM_HEIGHT - 1) {
             mvaddch(y + i, x + text_width, ACS_VLINE); 
        }
    }

    // --- LEFT SIDE: Text Info ---
    
    // Line 1: Name (Bold)
    attron(A_BOLD);
    mvprintw(y, x + 1, "%.*s", text_width - 2, m->name);
    attroff(A_BOLD);

    // Line 2: Current Value (Big)
    // Use different color for value
    if (!selected) attron(Terminal_colors[TEXT_BRIGHT]);
    mvprintw(y + 1, x + 1, "%.4f", m->current_value);
    if (!selected) attroff(Terminal_colors[TEXT_BRIGHT]);

    // Line 3: Min/Max (Small info)
    if (!selected) attron(Terminal_colors[TEXT_DIM]);
    mvprintw(y + 2, x + 1, "L:%.2f", m->min_value);
    mvprintw(y + 2, x + 10, "H:%.2f", m->max_value);
    if (!selected) attroff(Terminal_colors[TEXT_DIM]);

    // Line 4: Bottom Separator (Horizontal)
    // Draw a line across the whole item to separate from next item
    attron(Terminal_colors[PANEL_BORDER]);
    mvhline(y + ITEM_HEIGHT - 1, x, ACS_HLINE, w);
    // Fix the intersection with right border
    mvaddch(y + ITEM_HEIGHT - 1, x + w, ACS_RTEE);
    attroff(Terminal_colors[PANEL_BORDER]);

    // --- RIGHT SIDE: The Chart ---
    
    if (chart_w > 5) {
        int chart_color = selected ? Terminal_colors[TEXT_SELECTED] : Terminal_colors[GRAPH_LINE];
        
        // Draw chart in the top 3 rows (leave row 4 for separator)
        Sparkline_draw(m->history, m->history_count, 
                       y, chart_x, chart_w, ITEM_HEIGHT - 1, 
                       chart_color);
    }
    
    // Restore attributes
    attroff(bg_color);
}

Panel* MetricsPanel_new(int x, int y, int w, int h) {
    Panel* p = Panel_new(x, y, w, h, "Metrics [Live]");
    if (!p) return NULL;
    
    Panel_setItemHeight(p, ITEM_HEIGHT); // Set height to 4
    Panel_setDrawItem(p, MetricsPanel_drawItem);
    return p;
}

void MetricsPanel_addMetric(Panel* this, const char* name, float current_val, const float* values, int count) {
    if (!this) return;

    MetricData* m = calloc(1, sizeof(MetricData));
    m->name = strdup(name);
    m->current_value = current_val;
    m->history_count = count;
    m->history = malloc(count * sizeof(float));
    memcpy(m->history, values, count * sizeof(float));
    
    // Calc min/max for display
    m->min_value = values[0];
    m->max_value = values[0];
    for(int i=0; i<count; i++) {
        if(values[i] < m->min_value) m->min_value = values[i];
        if(values[i] > m->max_value) m->max_value = values[i];
    }

    Panel_addItem(this, name, m);
}

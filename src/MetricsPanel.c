#include "MetricsPanel.h"
#include "SparkLine.h"
#include "Terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define ITEM_HEIGHT 4

static void MetricsPanel_cleanup(void* data);

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
    
    // Ensure we stay within bounds
    if (w < 10) return; // Not enough space to draw anything meaningful
    
    // --- Draw containing box first ---
    int box_color = Terminal_colors[PANEL_BORDER];
    attron(box_color);
    
    // Top border
    mvhline(y, x, ACS_HLINE, w);
    
    // Side borders for rows 1-2
    for (int i = 1; i < ITEM_HEIGHT - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    
    // Bottom border
    mvhline(y + ITEM_HEIGHT - 1, x, ACS_HLINE, w);
    
    // Corners
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + ITEM_HEIGHT - 1, x, ACS_LLCORNER);
    mvaddch(y + ITEM_HEIGHT - 1, x + w - 1, ACS_LRCORNER);
    
    attroff(box_color);
    
    // --- Now draw content INSIDE the box ---
    int content_x = x + 1;
    int content_y = y + 1;
    int content_w = w - 2;
    
    // Layout calculations for inside content
    int text_width = 20;
    int chart_x = content_x + text_width + 1;
    int chart_w = content_w - text_width - 2;
    
    if (chart_w < 0) chart_w = 0;
    
    // Background fill for content area
    int bg_color = selected ? Terminal_colors[TEXT_SELECTED] : Terminal_colors[TEXT_NORMAL];
    attron(bg_color);
    
    // Fill only the content area (inside the box)
    for (int i = 0; i < ITEM_HEIGHT - 2; i++) {
        mvhline(content_y + i, content_x, ' ', content_w);
    }
    
    attroff(bg_color);
    
    // --- LEFT SIDE: Text Info ---
    attron(bg_color);
    
    // Line 1: Name (Bold)
    attron(A_BOLD);
    mvprintw(content_y, content_x, "%.*s", text_width - 1, m->name);
    attroff(A_BOLD);
    
    // Line 2: Current Value
    if (!selected) {
        attroff(bg_color);
        attron(Terminal_colors[TEXT_BRIGHT]);
    }
    mvprintw(content_y + 1, content_x, "%.4f", m->current_value);
    if (!selected) {
        attroff(Terminal_colors[TEXT_BRIGHT]);
        attron(bg_color);
    }
    
    attroff(bg_color);
    
    // Vertical separator between text and chart
    if (text_width < content_w - 2) {
        attron(Terminal_colors[PANEL_BORDER]);
        for (int i = 0; i < ITEM_HEIGHT - 2; i++) {
            mvaddch(content_y + i, content_x + text_width, ACS_VLINE);
        }
        attroff(Terminal_colors[PANEL_BORDER]);
    }
    
    // Min/Max on left side
    attron(selected ? bg_color : Terminal_colors[TEXT_DIM]);
    mvprintw(content_y + 1, content_x + 10, "L:%.2f H:%.2f", m->min_value, m->max_value);
    attroff(selected ? bg_color : Terminal_colors[TEXT_DIM]);
    
    // --- RIGHT SIDE: Chart ---
    if (chart_w > 5 && chart_x + chart_w <= x + w - 1) {
        int chart_color = selected ? Terminal_colors[TEXT_SELECTED] : Terminal_colors[GRAPH_LINE];
        
        // Draw chart in the available space (ITEM_HEIGHT - 2 for top/bottom borders)
        Sparkline_draw(m->history, m->history_count, 
                       content_y, chart_x, chart_w, ITEM_HEIGHT - 2, 
                       chart_color);
    }
}

Panel* MetricsPanel_new(int x, int y, int w, int h) {
    Panel* p = Panel_new(x, y, w, h, "Metrics [Live]");
    if (!p) return NULL;
    
    Panel_setItemHeight(p, ITEM_HEIGHT); // Set height to 4
    Panel_setDrawItem(p, MetricsPanel_drawItem);
    
    // Register cleanup
    Panel_setCleanupCallback(p, MetricsPanel_cleanup);

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

static void MetricsPanel_cleanup(void* data) {
    MetricData* m = (MetricData*)data;
    if (m) {
        free(m->name);
        free(m->history); // <--- The big array we were leaking
        free(m);
    }
}

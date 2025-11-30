#include "MetricsPanel.h"
#include "SparkLine.h"
#include "Terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <ncurses.h>

#define CARD_HEIGHT 12       
#define MIN_CARD_WIDTH 40    

// --- Internal Data Structures ---
typedef struct {
    char* name;
    float current_value;
    float min_value;
    float max_value;
    float* history;
    int history_count;
    int color_idx; 
} MetricData;

typedef struct {
    MetricData** metrics; 
    int count;            
} MetricRow;

typedef struct {
    MetricData** all_metrics; 
    int total_count;
    int capacity;
    int columns;          
    int selected_col;     
    int last_width;       
} MetricsState;

// --- Helper Functions ---
static void MetricsPanel_cleanupRow(void* data) {
    MetricRow* row = (MetricRow*)data;
    if (row) {
        free(row->metrics); 
        free(row);
    }
}

static void MetricsPanel_reflow(Panel* p) {
    MetricsState* state = (MetricsState*)Panel_getUserData(p);
    if (!state) return;

    state->last_width = p->w;
    state->columns = p->w / MIN_CARD_WIDTH;
    if (state->columns < 1) state->columns = 1;
    
    Panel_clear(p);
    Panel_setItemHeight(p, CARD_HEIGHT);

    for (int i = 0; i < state->total_count; i += state->columns) {
        MetricRow* row = malloc(sizeof(MetricRow));
        int remaining = state->total_count - i;
        row->count = (remaining < state->columns) ? remaining : state->columns;
        row->metrics = malloc(row->count * sizeof(MetricData*));
        for (int k = 0; k < row->count; k++) {
            row->metrics[k] = state->all_metrics[i + k];
        }
        Panel_addItem(p, "", row); 
    }
}

// --- Smart Axis Logic ---

// Returns a "Nice" step size (1, 2, 5, 10, 20, 50...)
static int calculate_nice_step(int range, int target_ticks) {
    if (target_ticks <= 0) target_ticks = 1;
    double raw_step = (double)range / target_ticks;
    
    // Calculate magnitude (power of 10)
    double mag = pow(10, floor(log10(raw_step)));
    double residual = raw_step / mag;
    
    int nice_step;
    if (residual > 5.0)      nice_step = 10 * (int)mag;
    else if (residual > 2.0) nice_step = 5 * (int)mag;
    else if (residual > 1.0) nice_step = 2 * (int)mag;
    else                     nice_step = 1 * (int)mag;
    
    if (nice_step < 1) nice_step = 1;
    return nice_step;
}

// --- Drawing Logic ---

static void draw_card(MetricData* m, int y, int x, int w, int h, bool selected) {
    // Colors
    int border_color = selected ? (Terminal_colors[TEXT_BRIGHT] | A_BOLD) 
                                : Terminal_colors[PANEL_BORDER];
    int text_color   = Terminal_colors[TEXT_NORMAL];
    int value_color  = Terminal_colors[TEXT_BRIGHT];
    int dim_color    = Terminal_colors[TEXT_DIM];
    
    int base_colors[] = {
        Terminal_colors[GRAPH_LINE],
        Terminal_colors[PANEL_HEADER],
        COLOR_PAIR(5),
        COLOR_PAIR(3) 
    };
    int color_pick = 0;
    if (m->name) color_pick = (m->name[0] + m->name[strlen(m->name)-1]) % 4; 
    int chart_color = base_colors[color_pick];

    // 1. Draw Container Border
    attron(border_color);
    mvhline(y, x, ACS_HLINE, w);
    mvhline(y + h - 1, x, ACS_HLINE, w);
    mvvline(y, x, ACS_VLINE, h);
    mvvline(y, x + w - 1, ACS_VLINE, h);
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    attroff(border_color);

    // 2. Header & Value
    attron(selected ? (Terminal_colors[PANEL_HEADER]) : (text_color | A_BOLD));
    mvprintw(y + 1, x + 2, "%.*s", w - 15, m->name);
    attroff(selected ? (Terminal_colors[PANEL_HEADER]) : (text_color | A_BOLD));

    attron(value_color | A_BOLD);
    char val_buf[32];
    snprintf(val_buf, 32, "%.2f", m->current_value);
    mvprintw(y + 1, x + w - 2 - strlen(val_buf), "%s", val_buf);
    attroff(value_color | A_BOLD);

   // 3. Y-AXIS LABELS
    attron(dim_color);
    mvprintw(y + 3, x + 2, "%4.1f", m->max_value);
    mvaddch(y + 3, x + 5, ACS_HLINE); 

    // Min label moved to (h-3) to separate from X-axis row
    mvprintw(y + h - 3, x + 2, "%4.1f", m->min_value);
    mvaddch(y + h - 3, x + 5, ACS_HLINE); 
    attroff(dim_color);

    // 4. CHART AREA
    int graph_h = h - 6; 
    int graph_w = w - 8; 
    int graph_x = x + 6; 
    int graph_y = y + 3; 

    if (graph_h > 1 && graph_w > 4) {
        // Clear background
        attron(text_color);
        for(int i=0; i<graph_h; i++) mvhline(graph_y+i, graph_x, ' ', graph_w);
        attroff(text_color);

        // Draw Y-Axis Line
        attron(dim_color);
        for(int i=0; i<graph_h; i++) mvaddch(graph_y+i, graph_x-1, ACS_VLINE);
        
        // Draw X-Axis Line (Tick marks will be added in X-Axis loop below)
        int axis_y = graph_y + graph_h;
        mvhline(axis_y, graph_x, ACS_HLINE, graph_w);
        mvaddch(axis_y, graph_x - 1, ACS_LLCORNER);

        // --- SMART X-AXIS LABELS ---
        int label_y = y + h - 2;
        int max_labels = graph_w / 10; // Approx 1 label every 10 chars
        int nice_step = calculate_nice_step(m->history_count, max_labels);
        
        int last_label_end_x = -1;

        for (int val = 0; val <= m->history_count; val += nice_step) {
            // Map value to pixel X
            double ratio = (double)val / m->history_count;
            if (m->history_count == 0) ratio = 0;
            
            // -1 because pixels are 0-indexed relative to width
            int px = (int)(ratio * (graph_w - 1));
            int screen_x = graph_x + px;

            // Draw Tick on Axis Line
            mvaddch(axis_y, screen_x, ACS_TTEE);

            // Format Number
            char buf[16];
            snprintf(buf, 16, "%d", val);
            int len = strlen(buf);

            // Calculate start X (Centered on tick)
            int start_x = screen_x - (len / 2);

            // Clamp left/right so we don't draw outside graph area
            if (start_x < graph_x) start_x = graph_x;
            if (start_x + len > graph_x + graph_w) start_x = graph_x + graph_w - len;

            // Collision Check: Ensure 2 spaces of gap
            if (start_x > last_label_end_x + 1) {
                mvprintw(label_y, start_x, "%s", buf);
                last_label_end_x = start_x + len;
            }
        }
        
        attroff(dim_color);
        
        // Draw Braille Line Chart
        Sparkline_draw(m->history, m->history_count, 
                       graph_y, graph_x, graph_w, graph_h, 
                       chart_color);
    }
}

static void MetricsPanel_drawItem(Panel* panel, int index, int y, int x, int w, bool row_selected) {
    MetricsState* state = (MetricsState*)Panel_getUserData(panel);
    
    if (state->last_width != w) {
        state->columns = w / MIN_CARD_WIDTH;
        if(state->columns < 1) state->columns = 1;
        state->last_width = w;
    }

    mvhline(y, x, ' ', w); 

    PanelItem* item = Panel_getItem(panel, index);
    if (!item || !item->data) return;
    MetricRow* row = (MetricRow*)item->data;

    int card_width = w / state->columns;
    
    for (int i = 0; i < row->count; i++) {
        int card_x = x + (i * card_width);
        int current_card_w = (i == state->columns - 1) ? (w - (i * card_width)) : card_width;
        
        if (i < row->count - 1) {
            current_card_w -= 1; 
            mvaddch(y, card_x + current_card_w, ' ');
        }

        bool is_card_focused = row_selected && (state->selected_col == i) && panel->has_focus;
        
        if (state->selected_col >= row->count && row_selected && panel->has_focus) {
             is_card_focused = (i == row->count - 1);
        }

        draw_card(row->metrics[i], y, card_x, current_card_w, CARD_HEIGHT, is_card_focused);
    }
}

static HandlerResult MetricsPanel_handleKey(Panel* p, int key) {
    MetricsState* state = (MetricsState*)Panel_getUserData(p);
    
    int current_row_idx = Panel_getSelectedIndex(p);
    int total_rows = Panel_getItemCount(p);
    PanelItem* item = Panel_getSelected(p);
    if (!item || !item->data) return IGNORED;
    MetricRow* current_row = (MetricRow*)item->data;

    if (key == 'm') {
        if (state->selected_col > 0) {
            state->selected_col--;
        } else {
            if (current_row_idx > 0) {
                Panel_setSelected(p, current_row_idx - 1);
                PanelItem* prev_item = Panel_getSelected(p);
                MetricRow* prev_row = (MetricRow*)prev_item->data;
                state->selected_col = prev_row->count - 1;
            }
        }
        Panel_setNeedsRedraw(p);
        return HANDLED; 
    }
    
    if (key == 'n') {
        if (state->selected_col < current_row->count - 1) {
            state->selected_col++;
        } else {
            if (current_row_idx < total_rows - 1) {
                Panel_setSelected(p, current_row_idx + 1);
                state->selected_col = 0;
            }
        }
        Panel_setNeedsRedraw(p);
        return HANDLED; 
    }
    
    return IGNORED;
}

Panel* MetricsPanel_new(int x, int y, int w, int h) {
    Panel* p = Panel_new(x, y, w, h, "Metrics [Grid]");
    if (!p) return NULL;

    MetricsState* state = calloc(1, sizeof(MetricsState));
    state->columns = 1;
    state->all_metrics = malloc(16 * sizeof(MetricData*));
    state->capacity = 16;
    state->total_count = 0;
    state->selected_col = 0;
    
    Panel_setUserData(p, state);
    Panel_setDrawItem(p, MetricsPanel_drawItem);
    Panel_setCleanupCallback(p, MetricsPanel_cleanupRow); 
    Panel_setEventHandler(p, MetricsPanel_handleKey);
    Panel_setItemHeight(p, CARD_HEIGHT);
    
    return p;
}

void MetricsPanel_addMetric(Panel* this, const char* name, float current_val, const float* values, int count) {
    if (!this) return;
    MetricsState* state = (MetricsState*)Panel_getUserData(this);

    if (Panel_getItemCount(this) == 0 && state->total_count > 0) {
        for(int i=0; i<state->total_count; i++) {
            if (state->all_metrics[i]) {
                free(state->all_metrics[i]->name);
                free(state->all_metrics[i]->history);
                free(state->all_metrics[i]);
            }
        }
        state->total_count = 0;
    }

    MetricData* m = calloc(1, sizeof(MetricData));
    m->name = strdup(name);
    m->current_value = current_val;
    m->history_count = count;
    m->history = malloc(count * sizeof(float));
    memcpy(m->history, values, count * sizeof(float));
    
    m->min_value = values[0];
    m->max_value = values[0];
    for(int i=0; i<count; i++) {
        if(values[i] < m->min_value) m->min_value = values[i];
        if(values[i] > m->max_value) m->max_value = values[i];
    }

    if (state->total_count >= state->capacity) {
        state->capacity *= 2;
        state->all_metrics = realloc(state->all_metrics, state->capacity * sizeof(MetricData*));
    }
    state->all_metrics[state->total_count++] = m;

    MetricsPanel_reflow(this);
}

void MetricsPanel_updateSize(Panel* p, int w, int h) {
    if (!p) return;
    MetricsState* state = (MetricsState*)Panel_getUserData(p);
    Panel_resize(p, w, h);
    if (state) {
        MetricsPanel_reflow(p);
    }
}

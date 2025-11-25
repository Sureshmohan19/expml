#include "ScreenManager.h"
#include "Panel.h"       // Now we use the real Panel functions
#include "FunctionBar.h" // Now we use the real FunctionBar
#include "Terminal.h"

#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// Internal struct to track panel layout preferences
typedef struct PanelLayout_ {
    Panel* panel;
    int requested_width; // 0 = auto/stretch
} PanelLayout;

struct ScreenManager_ {
    int x1, y1;
    int x2, y2;
    
    // We need to store the requested width alongside the panel pointer
    PanelLayout* layouts; 
    size_t panel_count;
    size_t focused;
    
    bool allow_focus_change;
    bool quit;
    
    char* header_text;
    FunctionBar* function_bar;
    
    double last_refresh;
    double refresh_interval;
    
    // Refresh Callback
    ScreenManager_OnRefresh on_refresh;
    void* refresh_userdata;
};

// Get current time in seconds
static double getCurrentTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

ScreenManager* ScreenManager_new(const char* header_text, double refresh_interval) {
    ScreenManager* this = (ScreenManager*)calloc(1, sizeof(ScreenManager));
    if (!this) return NULL;
    
    this->x1 = 0;
    this->y1 = 1;  // Header row
    this->x2 = 0;
    this->y2 = -2; // Footer rows (FunctionBar)
    
    this->layouts = NULL;
    this->panel_count = 0;
    this->focused = 0;
    
    this->allow_focus_change = true;
    this->quit = false;
    
    if (header_text) this->header_text = strdup(header_text);
    this->function_bar = NULL;
    
    this->last_refresh = getCurrentTime();
    this->refresh_interval = refresh_interval > 0.0 ? refresh_interval : 1.0;
    
    this->on_refresh = NULL;
    this->refresh_userdata = NULL;
    return this;
}

void ScreenManager_delete(ScreenManager* this) {
    if (!this) return;
    
    free(this->header_text);
    // FunctionBar is owned by caller usually, but if we wanted to own it:
    // FunctionBar_delete(this->function_bar); 
    
    free(this->layouts); // Don't free the panels inside, just the array
    free(this);
}

void ScreenManager_addPanel(ScreenManager* this, Panel* panel, int width) {
    if (!this || !panel) return;
    
    size_t new_count = this->panel_count + 1;
    PanelLayout* new_layouts = realloc(this->layouts, new_count * sizeof(PanelLayout));
    if (!new_layouts) return;
    
    this->layouts = new_layouts;
    this->layouts[this->panel_count].panel = panel;
    this->layouts[this->panel_count].requested_width = width;
    this->panel_count = new_count;
    
    // If this is the first panel, focus it
    if (this->panel_count == 1) {
        Panel_setFocus(panel, true);
    } else {
        Panel_setFocus(panel, false);
    }
    
    ScreenManager_resize(this);
}

void ScreenManager_resize(ScreenManager* this) {
    if (!this || this->panel_count == 0) return;
    
    int y_start = this->y1;
    int height = LINES + this->y2 - y_start + 1; // +1 because y2 is negative offset
    int total_width = COLS - this->x1 + this->x2;
    
    // 1. Calculate static widths
    int used_width = 0;
    int dynamic_panels = 0;
    
    for (size_t i = 0; i < this->panel_count; i++) {
        if (this->layouts[i].requested_width > 0) {
            used_width += this->layouts[i].requested_width;
        } else {
            dynamic_panels++;
        }
    }
    
    // 2. Calculate dynamic width
    int flexible_width = 0;
    if (dynamic_panels > 0) {
        flexible_width = (total_width - used_width) / dynamic_panels;
        if (flexible_width < 1) flexible_width = 1; // Minimum safety
    }
    
    // 3. Apply to panels
    int current_x = this->x1;
    
    for (size_t i = 0; i < this->panel_count; i++) {
        int w = this->layouts[i].requested_width;
        
        if (w <= 0) {
            // If it's the absolute last panel, give it all remaining pixels
            // to avoid rounding errors leaving gaps
            if (i == this->panel_count - 1) {
                w = COLS - current_x;
            } else {
                w = flexible_width;
            }
        }
        
        // Apply real resizing
        Panel_move(this->layouts[i].panel, current_x, y_start);
        Panel_resize(this->layouts[i].panel, w, height);
        
        current_x += w;
    }
}

bool ScreenManager_setFocus(ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return false;
    
    // Unfocus old
    Panel_setFocus(this->layouts[this->focused].panel, false);
    
    // Focus new
    this->focused = index;
    Panel_setFocus(this->layouts[this->focused].panel, true);
    
    return true;
}

void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* bar) {
    if (this) this->function_bar = bar;
}

void ScreenManager_setHeaderText(ScreenManager* this, const char* text) {
    if (!this) return;
    free(this->header_text);
    this->header_text = text ? strdup(text) : NULL;
}

void ScreenManager_setRefreshCallback(ScreenManager* this, ScreenManager_OnRefresh callback, void* userdata) {
    if (this) {
        this->on_refresh = callback;
        this->refresh_userdata = userdata;
    }
}

static void drawHeader(ScreenManager* this) {
    if (!this->header_text) return;
    
    attron(Terminal_colors[PANEL_HEADER]);
    mvhline(0, 0, ' ', COLS); 
    mvprintw(0, 0, " %s", this->header_text); // Added a space padding
    attroff(Terminal_colors[PANEL_HEADER]);
}

void ScreenManager_forceRedraw(ScreenManager* this) {
    if (!this) return;
    
    clear();
    drawHeader(this);
    
    // Draw all panels
    for (size_t i = 0; i < this->panel_count; i++) {
        Panel_draw(this->layouts[i].panel, true);
    }
    
    if (this->function_bar) {
        FunctionBar_draw(this->function_bar, COLS);
    }
    
    refresh();
}

int ScreenManager_run(ScreenManager* this) {
    if (!this) return -1;
    
    bool force_redraw = true;
    int ch = ERR;
    
    while (!this->quit) {
        // 1. Handle Input
        timeout(100); // Wait up to 100ms for key
        ch = Terminal_readKey();
        
        bool handled = false;
        
        if (ch != ERR) {
            // Global keys
            if (ch == KEY_RESIZE) {
                ScreenManager_resize(this);
                force_redraw = true;
                handled = true;
            } else if (ch == 'q' || ch == KEY_F(10)) {
                this->quit = true;
                handled = true;
            } else if (ch == '\t') { // Tab cycles focus
                size_t next = (this->focused + 1) % this->panel_count;
                ScreenManager_setFocus(this, next);
                force_redraw = true; // Focus change requires border redraw
                handled = true;
            } else if (ch == KEY_RIGHT && this->allow_focus_change) {
                 if (this->focused < this->panel_count - 1) {
                    ScreenManager_setFocus(this, this->focused + 1);
                    force_redraw = true;
                    handled = true;
                 }
            } else if (ch == KEY_LEFT && this->allow_focus_change) {
                 if (this->focused > 0) {
                    ScreenManager_setFocus(this, this->focused - 1);
                    force_redraw = true;
                    handled = true;
                 }
            }
            
            // If not global, pass to Focused Panel
            if (!handled && this->panel_count > 0) {
                Panel* p = this->layouts[this->focused].panel;
                // We assume Panel_onKey returns bool (true if handled)
                // You can expand this if Panel_onKey returns a complex struct
                if (Panel_onKey(p, ch)) {
                    // If panel consumed the key, it likely needs a redraw
                    // Panel handles its own internal state, but we might need to refresh screen
                }
            }
        }
        
        // 2. Update Data (Auto-Refresh)
        double current_time = getCurrentTime();
        if ((current_time - this->last_refresh) >= this->refresh_interval) {
            this->last_refresh = current_time;
            
            // Call the user-defined refresh callback
            if (this->on_refresh) {
                this->on_refresh(this->refresh_userdata);
            }
            
            // Mark panels for redraw
            for(size_t i=0; i<this->panel_count; i++) {
                Panel_setNeedsRedraw(this->layouts[i].panel);
            }
            force_redraw = true; // Ensure screen updates
        }

        // 3. Draw
        drawHeader(this);
        
        for (size_t i = 0; i < this->panel_count; i++) {
            // Panel_draw checks internal 'needs_redraw' flag, so it's efficient to call often
            Panel_draw(this->layouts[i].panel, force_redraw);
        }
        
        if (this->function_bar) {
            FunctionBar_draw(this->function_bar, COLS);
        }
        
        refresh();
        force_redraw = false;
    }
    
    return 0;
}

Panel* ScreenManager_getPanel(const ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return NULL;
    return this->layouts[index].panel;
}

#include "Panel.h"
#include "Header.h"
#include "Terminal.h"
#include "Constants.h" 
#include "FunctionBar.h" 
#include "ScreenManager.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/time.h>

typedef struct PanelLayout_ {
    Panel* panel;
    int requested_width; // 0 = auto/stretch
} PanelLayout;

struct ScreenManager_ {
    int x1, y1;
    int x2, y2;
    PanelLayout* layouts; 
    size_t panel_count;
    size_t focused;
    bool allow_focus_change;
    bool quit;
    bool show_help;
    Header* header;
    FunctionBar* function_bar;
    double last_refresh;
    double refresh_interval;
    ScreenManager_OnRefresh on_refresh;
    void* refresh_userdata;
};

// Returns current time in seconds with microsecond precision
static double getCurrentTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// Draws the instruction hint line below the header
static void drawInstructions(void) {
    int instruction_y = 4;
    attron(Terminal_colors[TEXT_DIM]);
    mvhline(instruction_y, 0, ' ', COLS);
    mvhline(instruction_y + 1, 0, ' ', COLS);
    attroff(Terminal_colors[TEXT_DIM]);

    attron(Terminal_colors[TEXT_BRIGHT]);
    mvprintw(instruction_y, 2, "Hint:");
    attroff(Terminal_colors[TEXT_BRIGHT]);

    // Line 1: Navigation
    attron(Terminal_colors[TEXT_DIM]);
    mvprintw(instruction_y + 1, 2, "/");
    attron(A_BOLD);
    printw(" press Tab ");
    attroff(A_BOLD);
    printw("to switch panels");
    attroff(Terminal_colors[TEXT_DIM]);

    // Line 2: Manual Refresh (New)
    attron(Terminal_colors[TEXT_DIM]);
    mvprintw(instruction_y + 2, 2, "/");
    attron(A_BOLD);
    printw(" press Ctrl+L ");
    attroff(A_BOLD);
    printw("for manual refresh");
    attroff(Terminal_colors[TEXT_DIM]);
}

// Creates a new ScreenManager with header and refresh interval
ScreenManager* ScreenManager_new(const char* header_text, double refresh_interval) {
    ScreenManager* this = (ScreenManager*)calloc(1, sizeof(ScreenManager));
    if (!this) return NULL;
    
    this->x1 = 1;
    this->y1 = 8;
    this->x2 = -1;
    this->y2 = -2;
    this->layouts = NULL;
    this->panel_count = 0;
    this->focused = 0;
    this->allow_focus_change = true;
    this->quit = false;
    this->show_help = false;
    this->function_bar = NULL;
    this->header = Header_new(header_text);
    this->last_refresh = getCurrentTime();
    this->refresh_interval = refresh_interval > 0.0 ? refresh_interval : 1.0;
    this->on_refresh = NULL;
    this->refresh_userdata = NULL;
    return this;
}

// Frees all resources associated with the ScreenManager
void ScreenManager_delete(ScreenManager* this) {
    if (!this) return;

    Header_delete(this->header);
    
    // Free all panels before freeing the layouts array
    for (size_t i = 0; i < this->panel_count; i++) {
        Panel_delete(this->layouts[i].panel);
    }
    
    free(this->layouts);
    free(this);
}

// Adds a panel to the layout with specified width (0 = auto-stretch)
void ScreenManager_addPanel(ScreenManager* this, Panel* panel, int width) {
    if (!this || !panel) return;
    
    size_t new_count = this->panel_count + 1;
    PanelLayout* new_layouts = realloc(this->layouts, new_count * sizeof(PanelLayout));
    if (!new_layouts) return;
    
    this->layouts = new_layouts;
    this->layouts[this->panel_count].panel = panel;
    this->layouts[this->panel_count].requested_width = width;
    this->panel_count = new_count;
    
    if (this->panel_count == 1) {
        Panel_setFocus(panel, true);
    } else {
        Panel_setFocus(panel, false);
    }
    ScreenManager_resize(this);
}

// Removes a panel at the specified index and returns it (caller must delete it)
Panel* ScreenManager_removePanel(ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return NULL;
    
    Panel* removed = this->layouts[index].panel;
    
    // Shift remaining panels left
    if (index < this->panel_count - 1) {
        memmove(&this->layouts[index], 
                &this->layouts[index + 1], 
                (this->panel_count - index - 1) * sizeof(PanelLayout));
    }
    
    this->panel_count--;
    
    // Adjust focus if needed
    if (this->focused >= this->panel_count && this->panel_count > 0) {
        this->focused = this->panel_count - 1;
    }
    
    // Update focus states for all remaining panels
    for (size_t i = 0; i < this->panel_count; i++) {
        Panel_setFocus(this->layouts[i].panel, i == this->focused);
    }
    
    ScreenManager_resize(this);
    return removed;
}

// Recalculates and applies layout to all panels based on terminal size
void ScreenManager_resize(ScreenManager* this) {
    if (!this || this->panel_count == 0) return;
    
    int y_start = this->y1;
    int height = LINES + this->y2 - y_start + 1; 
    int total_available_width = COLS - this->x1 + this->x2;
    
    // --- Step 1: Analyze Requests ---
    int total_fixed_requested = 0;
    int dynamic_panels = 0;
    
    for (size_t i = 0; i < this->panel_count; i++) {
        if (this->layouts[i].requested_width > 0) {
            total_fixed_requested += this->layouts[i].requested_width;
        } else {
            dynamic_panels++;
        }
    }
    
    // --- Step 2: Calculate Compression ---
    // We want to ensure dynamic panels get at least MAIN_PANEL_MIN_WIDTH
    double compression_ratio = 1.0;
    
    // If we have dynamic panels, check if they are being squeezed too much
    if (dynamic_panels > 0) {
        int space_for_dynamic = total_available_width - total_fixed_requested;
        
        if (space_for_dynamic < MAIN_PANEL_MIN_WIDTH) {
            // Not enough space! We need to shrink the fixed panels.
            // Calculate how much space fixed panels are ALLOWED to take.
            int max_allowed_fixed = total_available_width - MAIN_PANEL_MIN_WIDTH;
            
            // Calculate ratio to scale fixed panels down
            if (total_fixed_requested > 0) {
                compression_ratio = (double)max_allowed_fixed / total_fixed_requested;
            }
        }
    }

    // --- Step 3: Apply Layout ---
    // First pass: Calculate actual used width for fixed panels based on compression
    int* calculated_widths = malloc(this->panel_count * sizeof(int));
    int final_fixed_total = 0;
    
    for (size_t i = 0; i < this->panel_count; i++) {
        int req = this->layouts[i].requested_width;
        if (req > 0) {
            // Apply compression
            int w = (int)(req * compression_ratio);
            
            // Enforce hard minimum for sidebars
            if (w < SIDEBAR_MIN_WIDTH) w = SIDEBAR_MIN_WIDTH;
            
            // Safety: Don't exceed total screen width
            if (w > total_available_width) w = total_available_width;

            calculated_widths[i] = w;
            final_fixed_total += w;
        } else {
            calculated_widths[i] = 0; // Dynamic, calculate next
        }
    }

    // Calculate space remaining for dynamic panels
    int remaining_width = total_available_width - final_fixed_total;
    int flexible_width = 0;
    if (dynamic_panels > 0) {
        if (remaining_width < 1) remaining_width = 1; // Prevent 0 width
        flexible_width = remaining_width / dynamic_panels;
    }

    // Second pass: Position and resize everything
    int current_x = this->x1;    
    for (size_t i = 0; i < this->panel_count; i++) {
        int w = calculated_widths[i];
        
        if (this->layouts[i].requested_width <= 0) {
            // This is a dynamic panel
            if (i == this->panel_count - 1 && dynamic_panels == 1) {
                // Last panel gets the rest of the screen (fixes rounding errors)
                w = total_available_width - (current_x - this->x1);
            } else {
                w = flexible_width;
            }
        }

        bool draw_right_sep = (i < this->panel_count - 1);
        Panel_move(this->layouts[i].panel, current_x, y_start);
        Panel_resize(this->layouts[i].panel, w, height);
        Panel_setDrawRightSeparator(this->layouts[i].panel, draw_right_sep);
        current_x += w;
    }
    
    free(calculated_widths);
}

// Sets focus to the panel at the specified index
bool ScreenManager_setFocus(ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return false;
    Panel_setFocus(this->layouts[this->focused].panel, false);
    this->focused = index;
    Panel_setFocus(this->layouts[this->focused].panel, true);
    
    return true;
}

// Sets the function bar to be displayed at the bottom
void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* bar) {
    if (this) this->function_bar = bar;
}

// Sets the refresh callback and user data for periodic updates
void ScreenManager_setRefreshCallback(ScreenManager* this, ScreenManager_OnRefresh callback, void* userdata) {
    if (this) {
        this->on_refresh = callback;
        this->refresh_userdata = userdata;
    }
}

// Forces a complete redraw of the entire screen
void ScreenManager_forceRedraw(ScreenManager* this) {
    if (!this) return;
    
    clear();
    Header_draw(this->header);
    drawInstructions();
    
    for (size_t i = 0; i < this->panel_count; i++) {
        Panel_draw(this->layouts[i].panel, true);
    }
    
    if (this->function_bar) {
        FunctionBar_draw(this->function_bar, COLS);
    }
    
    refresh();
}

// Draws the help modal overlay
static void drawHelp(ScreenManager* this) {
    (void)this;  // Suppress unused parameter warning
    int w = 50;
    int h = 16;
    int x = (COLS - w) / 2;
    int y = (LINES - h) / 2;

    attron(Terminal_colors[PANEL_BACKGROUND]);
    for (int i = 0; i < h; i++) {
        mvhline(y + i, x, ' ', w);
    }
    
    attron(Terminal_colors[PANEL_BORDER_ACTIVE]);
    mvhline(y, x, ACS_HLINE, w);
    mvhline(y + h - 1, x, ACS_HLINE, w);
    mvvline(y, x, ACS_VLINE, h);
    mvvline(y, x + w - 1, ACS_VLINE, h);
    
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    
    attron(A_BOLD);
    mvprintw(y, x + 2, " Help ");
    attroff(A_BOLD);
    attroff(Terminal_colors[PANEL_BORDER_ACTIVE]);

    int text_x = x + 4;
    int text_y = y + 2;
    
    attron(Terminal_colors[TEXT_NORMAL]);    
    attron(A_BOLD); mvprintw(text_y++, text_x, "Navigation"); attroff(A_BOLD);
    
    text_y++;
    mvprintw(text_y++, text_x, "  TAB / Arrows : Switch Panels");
    mvprintw(text_y++, text_x, "  Down / Up    : Scroll Down/Up");
    mvprintw(text_y++, text_x, "  PgUp / PgDn  : Scroll Page");
    mvprintw(text_y++, text_x, "  Home / End   : Jump to Top/Bottom");
    
    text_y++;
    attron(A_BOLD); mvprintw(text_y++, text_x, "General"); attroff(A_BOLD);
    text_y++;
    mvprintw(text_y++, text_x, "  h            : Help");
    mvprintw(text_y++, text_x, "  q            : Quit");
    mvprintw(text_y++, text_x, "  Ctrl+L       : Force Redraw");
    
    text_y++;

    attron(Terminal_colors[TEXT_DIM]);
    mvprintw(text_y++, text_x, "Press any key to close...");
    attroff(Terminal_colors[TEXT_DIM]);
    attroff(Terminal_colors[TEXT_NORMAL]);
    attroff(Terminal_colors[PANEL_BACKGROUND]);
}

// Main event loop - handles input, refresh, and rendering
int ScreenManager_run(ScreenManager* this) {
    if (!this) return -1;
    bool force_redraw = true;
    int ch = ERR;
    
    while (!this->quit) {
        timeout(100);
        ch = Terminal_readKey();
        
        if (this->show_help) {
            if (ch != ERR) {
                if (ch == KEY_RESIZE) {
                    ScreenManager_resize(this);
                } else {
                    this->show_help = false;
                }
                force_redraw = true;
            }
        } 
        else if (ch != ERR) {
            bool handled = false;
            
            // 1. Global Keys (Highest Priority)
            if (ch == KEY_RESIZE) {
                endwin();  // Temporarily exit ncurses mode
                refresh(); // Restore it (this forces ncurses to re-read terminal dims)

                Terminal_resetColors(); 
                clear();
                ScreenManager_resize(this);
                force_redraw = true;
                handled = true;
            } else if (ch == 12) { // Ctrl+L (Force Redraw)
                endwin();
                refresh();
                Terminal_resetColors(); // Restore RGB colors
                clear();                // Wipe artifacts
                ScreenManager_resize(this);
                force_redraw = true;
                handled = true;
            } else if (ch == 'q') {
                this->quit = true;
                handled = true;
            } else if (ch == 'h') {
                this->show_help = true;
                force_redraw = true;
                handled = true;
            } else if (ch == '\t') { 
                size_t next = (this->focused + 1) % this->panel_count;
                ScreenManager_setFocus(this, next);
                force_redraw = true; 
                handled = true;
            }

            // 2. Offer key to the Focused Panel (Edge Bumping Logic)
            // If the panel uses the key (e.g., Metrics moving selection), it returns true.
            // If the panel hits an edge or doesn't use it, it returns false.
            if (!handled && this->panel_count > 0) {
                Panel* p = this->layouts[this->focused].panel;
                if (Panel_onKey(p, ch)) {
                    handled = true; // Panel consumed the key
                }
            }
            
            // 3. ScreenManager Navigation (Fallback)
            // If the panel ignored the arrow key (e.g., RunPanel, or Metrics at edge),
            // we switch focus here.
            if (!handled) {
                if (ch == KEY_RIGHT && this->allow_focus_change) {
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
            }
        }
        
        // Check if it's time for a periodic refresh
        double current_time = getCurrentTime();
        if ((current_time - this->last_refresh) >= this->refresh_interval) { 
            this->last_refresh = current_time;

            if (this->on_refresh) { this->on_refresh(this->refresh_userdata); }
            for(size_t i=0; i<this->panel_count; i++) {
                Panel_setNeedsRedraw(this->layouts[i].panel);
            }
            force_redraw = true;
        }
        
        // Render everything
        Header_draw(this->header);
        drawInstructions();
        
        for (size_t i = 0; i < this->panel_count; i++) {
            Panel_draw(this->layouts[i].panel, force_redraw);
        }

        if (this->function_bar) { FunctionBar_draw(this->function_bar, COLS); }
        if (this->show_help) { drawHelp(this); }
        refresh();
        force_redraw = false;
    }
    return 0;
}

// Returns the panel at the specified index
Panel* ScreenManager_getPanel(const ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return NULL;
    return this->layouts[index].panel;
}

// Returns the currently focused panel
Panel* ScreenManager_getFocused(ScreenManager* this) {
    if (!this || this->panel_count == 0) return NULL;
    return this->layouts[this->focused].panel;
}

// Returns the total number of panels
size_t ScreenManager_getPanelCount(const ScreenManager* this) {
    if (!this) return 0;
    return this->panel_count;
}

// Sets the header title text
void ScreenManager_setHeaderText(ScreenManager* this, const char* text) {
    if (this) Header_setTitle(this->header, text);
}

// Sets the header status text
void ScreenManager_setHeaderStatus(ScreenManager* this, const char* status) {
    if (this) Header_setStatus(this->header, status);
}

// Sets the header runtime value
void ScreenManager_setHeaderRuntime(ScreenManager* this, double runtime) {
    if (this) Header_setRuntime(this->header, runtime);
}

// Placeholder functions for potential future use
void ScreenManager_setStartTime(ScreenManager* this, const char* time) {
    // Not currently used, reserved for future functionality
    (void)this;
    (void)time;
}

void ScreenManager_setEndTime(ScreenManager* this, const char* time) {
    // Not currently used, reserved for future functionality
    (void)this;
    (void)time;
}
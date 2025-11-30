#include "Panel.h"
#include "Header.h"
#include "Terminal.h"
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

static double getCurrentTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

ScreenManager* ScreenManager_new(const char* header_text, double refresh_interval) {
    ScreenManager* this = (ScreenManager*)calloc(1, sizeof(ScreenManager));
    if (!this) return NULL;
    
    this->x1 = 1;
    this->y1 = 7;
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

void ScreenManager_delete(ScreenManager* this) {
    if (!this) return;

    Header_delete(this->header);
    free(this->layouts);
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
    int height = LINES + this->y2 - y_start + 1; 
    int total_width = COLS - this->x1 + this->x2;
    int used_width = 0;
    int dynamic_panels = 0;
    
    for (size_t i = 0; i < this->panel_count; i++) {
        if (this->layouts[i].requested_width > 0) {
            used_width += this->layouts[i].requested_width;
        } else {
            dynamic_panels++;
        }
    }
    
    int flexible_width = 0;
    if (dynamic_panels > 0) {
        flexible_width = (total_width - used_width) / dynamic_panels;
        if (flexible_width < 1) flexible_width = 1; // Minimum safety
    }

    int current_x = this->x1;    
    for (size_t i = 0; i < this->panel_count; i++) {
        int w = this->layouts[i].requested_width;
        
        if (w <= 0) {
            if (i == this->panel_count - 1) {
                w = COLS - current_x;
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
}

bool ScreenManager_setFocus(ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return false;
    Panel_setFocus(this->layouts[this->focused].panel, false);
    this->focused = index;
    Panel_setFocus(this->layouts[this->focused].panel, true);
    
    return true;
}

void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* bar) {
    if (this) this->function_bar = bar;
}

void ScreenManager_setRefreshCallback(ScreenManager* this, ScreenManager_OnRefresh callback, void* userdata) {
    if (this) {
        this->on_refresh = callback;
        this->refresh_userdata = userdata;
    }
}

void ScreenManager_forceRedraw(ScreenManager* this) {
    if (!this) return;
    
    clear();
    Header_draw(this->header);
    
   // Add instruction line below header
    int instruction_y = 4;
    attron(Terminal_colors[TEXT_DIM]);
    mvhline(instruction_y, 0, ' ', COLS);
    mvhline(instruction_y + 1, 0, ' ', COLS);
    attroff(Terminal_colors[TEXT_DIM]);  // Turn OFF dim before turning ON bright

    attron(Terminal_colors[TEXT_BRIGHT]);
    mvprintw(instruction_y, 2, "Hint:");
    attroff(Terminal_colors[TEXT_BRIGHT]);  // Turn OFF bright

    attron(Terminal_colors[TEXT_DIM]);
    mvprintw(instruction_y + 1, 2, "/");
    attron(A_BOLD);
    printw(" press Tab ");
    attroff(A_BOLD);
    printw("to switch panels");
    attroff(Terminal_colors[TEXT_DIM]);

    for (size_t i = 0; i < this->panel_count; i++) {
        Panel_draw(this->layouts[i].panel, true);
    }
    
    if (this->function_bar) {
        FunctionBar_draw(this->function_bar, COLS);
    }
    
    refresh();
}

static void drawHelp(ScreenManager* this) {
    int w = 50;
    int h = 14;
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
    mvprintw(text_y++, text_x, "  j / k        : Scroll Down/Up");
    mvprintw(text_y++, text_x, "  Down / Up    : Scroll Down/Up");
    mvprintw(text_y++, text_x, "  PgUp / PgDn  : Scroll Page");
    mvprintw(text_y++, text_x, "  Home / End   : Jump to Top/Bottom");
    
    text_y++;
    attron(A_BOLD); mvprintw(text_y++, text_x, "General"); attroff(A_BOLD);
    text_y++;
    mvprintw(text_y++, text_x, "  h            : Help");
    mvprintw(text_y++, text_x, "  q            : Quit");
    
    text_y++;

    attron(Terminal_colors[TEXT_DIM]);
    mvprintw(text_y++, text_x, "Press any key to close...");
    attroff(Terminal_colors[TEXT_DIM]);
    attroff(Terminal_colors[TEXT_NORMAL]);
    attroff(Terminal_colors[PANEL_BACKGROUND]);
}

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
            if (ch == KEY_RESIZE) {
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
        
            if (!handled && this->panel_count > 0) {
                Panel* p = this->layouts[this->focused].panel;
                if (Panel_onKey(p, ch)) {
                }
            }
        }
        
        double current_time = getCurrentTime();
        if ((current_time - this->last_refresh) >= this->refresh_interval) { 
            this->last_refresh = current_time;

            if (this->on_refresh) { this->on_refresh(this->refresh_userdata); }
            for(size_t i=0; i<this->panel_count; i++) {
                Panel_setNeedsRedraw(this->layouts[i].panel);
            }
            force_redraw = true; // Ensure screen updates
        }
        
        Header_draw(this->header);

        // Add instruction line below header
        int instruction_y = 4;
        attron(Terminal_colors[TEXT_DIM]);
        mvhline(instruction_y, 0, ' ', COLS);
        mvhline(instruction_y + 1, 0, ' ', COLS);
        attroff(Terminal_colors[TEXT_DIM]);  // Turn OFF dim before turning ON bright

        attron(Terminal_colors[TEXT_BRIGHT]);
        mvprintw(instruction_y, 2, "Hint:");
        attroff(Terminal_colors[TEXT_BRIGHT]);  // Turn OFF bright

        attron(Terminal_colors[TEXT_DIM]);
        mvprintw(instruction_y + 1, 2, "/");
        attron(A_BOLD);
        printw(" press Tab ");
        attroff(A_BOLD);
        printw("to switch panels");
        attroff(Terminal_colors[TEXT_DIM]);
        
        for (size_t i = 0; i < this->panel_count; i++) {
            Panel_draw(this->layouts[i].panel, force_redraw);
        }

        if (this->function_bar) { FunctionBar_draw(this->function_bar, COLS); }
        if (this->show_help) { drawHelp(this); }
        refresh();  // Move to here - after drawing everything
        force_redraw = false;
        }
    return 0;
}

Panel* ScreenManager_getPanel(const ScreenManager* this, size_t index) {
    if (!this || index >= this->panel_count) return NULL;
    return this->layouts[index].panel;
}                                           

void ScreenManager_setHeaderText(ScreenManager* this, const char* text) {
    if (this) Header_setTitle(this->header, text);
}

void ScreenManager_setHeaderStatus(ScreenManager* this, const char* status) {
    if (this) Header_setStatus(this->header, status);
}

void ScreenManager_setHeaderRuntime(ScreenManager* this, double runtime) {
    if (this) Header_setRuntime(this->header, runtime);
}
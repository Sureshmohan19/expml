#include "Header.h"
#include "Terminal.h"

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

struct Header_ {
    char* title;
    char* status;
    double runtime;
};

Header* Header_new(const char* title) {
    Header* this = calloc(1, sizeof(Header));
    if (!this) return NULL;
    this->title = title ? strdup(title) : NULL;
    this->status = NULL;
    this->runtime = 0.0;
    return this;
}

void Header_delete(Header* this) {
    if (!this) return;
    free(this->title);
    free(this->status);
    free(this);
}

void Header_setTitle(Header* this, const char* title) {
    if (!this) return;
    free(this->title);
    this->title = title ? strdup(title) : NULL;
}

void Header_setStatus(Header* this, const char* status) {
    if (!this) return;
    free(this->status);
    this->status = status ? strdup(status) : NULL;
}

void Header_setRuntime(Header* this, double runtime) {
    if (!this) return;
    this->runtime = runtime;
}

void Header_draw(const Header* this) {
    if (!this) return;
    
    int border_color = Terminal_colors[GRAPH_LINE];
    int title_color = A_BOLD | Terminal_colors[GRAPH_LINE];
    int subtitle_color = Terminal_colors[TEXT_DIM];
    
    // Top border
    attron(border_color);
    mvaddch(0, 0, ACS_ULCORNER);
    mvhline(0, 1, ACS_HLINE, COLS - 2);
    mvaddch(0, COLS - 1, ACS_URCORNER);
    attroff(border_color);
    
    // Content line 1
    attron(border_color);
    mvaddch(1, 0, ACS_VLINE);
    mvaddch(1, COLS - 1, ACS_VLINE);
    attroff(border_color);
    
    mvhline(1, 1, ' ', COLS - 2);
    attron(title_color);
    mvprintw(1, 2, "expml v0.1.0");
    attroff(title_color);

    if (this->title) {
    attron(Terminal_colors[TEXT_NORMAL]);
    mvprintw(1, COLS - strlen(this->title) - 3, "%s", this->title);
    attroff(Terminal_colors[TEXT_NORMAL]);
}
    
    // Content line 2 - subtitle on left, status and runtime on right
    attron(border_color);
    mvaddch(2, 0, ACS_VLINE);
    mvaddch(2, COLS - 1, ACS_VLINE);
    attroff(border_color);
    
    mvhline(2, 1, ' ', COLS - 2);
    attron(subtitle_color);
    mvprintw(2, 2, "terminal-based ML experiment tracker 🎧");
    attroff(subtitle_color);
    
    // Right side: Status and Runtime
    if (this->status || this->runtime > 0) {
        char info[128];
        if (this->status && this->runtime > 0) {
            snprintf(info, sizeof(info), "status: %s | runtime: %.0fs", 
                     this->status, this->runtime);
        } else if (this->status) {
            snprintf(info, sizeof(info), "status: %s", this->status);
        } else {
            snprintf(info, sizeof(info), "runtime: %.0fs", this->runtime);
        }
        
        attron(Terminal_colors[TEXT_DIM]);
        mvprintw(2, COLS - strlen(info) - 3, "%s", info);
        attroff(Terminal_colors[TEXT_DIM]);
    }
    
    // Bottom border
    attron(border_color);
    mvaddch(3, 0, ACS_LLCORNER);
    mvhline(3, 1, ACS_HLINE, COLS - 2);
    mvaddch(3, COLS - 1, ACS_LRCORNER);
    attroff(border_color);
}
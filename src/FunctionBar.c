#include "FunctionBar.h"
#include "Terminal.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_CONTEXT_LEN 256
#define MAX_KEYS 15

struct FunctionBar_ {
    char* keys[MAX_KEYS];
    char* labels[MAX_KEYS];
    int count;
    char context[MAX_CONTEXT_LEN];
};

FunctionBar* FunctionBar_new(const char* const* keys, const char* const* labels) {
    FunctionBar* this = calloc(1, sizeof(FunctionBar));
    if (!this) return NULL;

    int i = 0;
    while (keys[i] && labels[i] && i < MAX_KEYS) {
        this->keys[i] = strdup(keys[i]);
        this->labels[i] = strdup(labels[i]);
        i++;
    }
    this->count = i;
    this->context[0] = '\0'; // Empty context by default
    return this;
}

void FunctionBar_delete(FunctionBar* this) {
    if (!this) return;
    for (int i = 0; i < this->count; i++) {
        free(this->keys[i]);
        free(this->labels[i]);
    }
    free(this);
}

void FunctionBar_setContext(FunctionBar* this, const char* fmt, ...) {
    if (!this) return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(this->context, MAX_CONTEXT_LEN, fmt, args);
    va_end(args);
}

void FunctionBar_draw(const FunctionBar* this, int width) {
    if (!this) return;
    int y = LINES - 2; 
    int bar_color = Terminal_colors[STATUS_BAR];

    attron(bar_color);
    mvhline(y, 0, ' ', width);
    attroff(bar_color);

    attron(bar_color);
    mvprintw(y, 1, "%s", this->context);
    attroff(bar_color);

    int current_x = width - 1;

    for (int i = this->count - 1; i >= 0; i--) {
        int key_len = strlen(this->keys[i]);
        int label_len = strlen(this->labels[i]);
        int total_len = key_len + label_len + 2; // +2 for ": "

        current_x -= total_len;
        if (current_x < (int)strlen(this->context) + 3) break;

        attron(bar_color | A_BOLD);
        mvprintw(y, current_x, "%s", this->keys[i]);
        attroff(A_BOLD);
        
        attron(bar_color);
        printw(":%s", this->labels[i]);
        attroff(bar_color);

        current_x -= 2; 
    }

    attroff(bar_color);
    refresh();
}
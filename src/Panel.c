#include "Panel.h"
#include "Terminal.h"

#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX(x, min), max))

Panel* Panel_new(int x, int y, int w, int h, const char* header) {
    Panel* this = (Panel*)calloc(1, sizeof(Panel));
    if (!this) {
        return NULL;
    }
    
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    
    if (header) {
        this->header = strdup(header);
    } else {
        this->header = NULL;
    }
    
    this->items = (PanelItem*)malloc(INITIAL_CAPACITY * sizeof(PanelItem));
    if (!this->items) {
        free(this->header);
        free(this);
        return NULL;
    }
    
    this->item_count = 0;
    this->item_capacity = INITIAL_CAPACITY;
    
    this->selected = 0;
    this->scroll_v = 0;
    this->scroll_h = 0;
    this->item_height = 1;
    this->needs_redraw = true;
    this->has_focus = false;
    
    this->event_handler = NULL;
    this->draw_item = NULL;
    this->cleanup_item = NULL;
    this->user_data = NULL;
    
    return this;
}

void Panel_delete(Panel* this) {
    if (!this) return;
    
    free(this->header);
    
    // Use the same logic as clear to free items
    for (size_t i = 0; i < this->item_count; i++) {
        if (this->cleanup_item && this->items[i].data) {
            this->cleanup_item(this->items[i].data);
        }
        free(this->items[i].text);
    }
    free(this->items);
    
    free(this);
}

void Panel_setEventHandler(Panel* this, Panel_EventHandler handler) {
    if (this) {
        this->event_handler = handler;
    }
}

void Panel_setDrawItem(Panel* this, Panel_DrawItem draw_item) {
    if (this) {
        this->draw_item = draw_item;
    }
}

void Panel_setUserData(Panel* this, void* user_data) {
    if (this) {
        this->user_data = user_data;
    }
}

void* Panel_getUserData(Panel* this) {
    return this ? this->user_data : NULL;
}

void Panel_setHeader(Panel* this, const char* header) {
    if (!this) {
        return;
    }
    
    free(this->header);
    this->header = header ? strdup(header) : NULL;
    this->needs_redraw = true;
}

void Panel_move(Panel* this, int x, int y) {
    if (!this) {
        return;
    }
    
    this->x = x;
    this->y = y;
    this->needs_redraw = true;
}

void Panel_resize(Panel* this, int w, int h) {
    if (!this) {
        return;
    }
    
    this->w = w;
    this->h = h;
    this->needs_redraw = true;
}

void Panel_setItemHeight(Panel* this, int h) {
    if (this && h > 0) {
        this->item_height = h;
        this->needs_redraw = true;
    }
}

void Panel_setCleanupCallback(Panel* this, Panel_ItemCleanup callback) {
    if (this) this->cleanup_item = callback;
}

int Panel_addItem(Panel* this, const char* text, void* data) {
    if (!this || !text) {
        return -1;
    }
    
    // Grow array if needed
    if (this->item_count >= this->item_capacity) {
        size_t new_capacity = this->item_capacity * 2;
        PanelItem* new_items = (PanelItem*)realloc(this->items, 
                                                    new_capacity * sizeof(PanelItem));
        if (!new_items) {
            return -1;
        }
        this->items = new_items;
        this->item_capacity = new_capacity;
    }
    
    // Add item
    int index = this->item_count;
    this->items[index].text = strdup(text);
    this->items[index].data = data;
    this->item_count++;
    this->needs_redraw = true;
    
    return index;
}

void Panel_insertItem(Panel* this, int index, const char* text, void* data) {
    if (!this || !text || index < 0) {
        return;
    }
    
    if (index >= (int)this->item_count) {
        Panel_addItem(this, text, data);
        return;
    }
    
    // Grow array if needed
    if (this->item_count >= this->item_capacity) {
        size_t new_capacity = this->item_capacity * 2;
        PanelItem* new_items = (PanelItem*)realloc(this->items, 
                                                    new_capacity * sizeof(PanelItem));
        if (!new_items) {
            return;
        }
        this->items = new_items;
        this->item_capacity = new_capacity;
    }
    
    // Shift items right
    memmove(&this->items[index + 1], &this->items[index], 
            (this->item_count - index) * sizeof(PanelItem));
    
    // Insert new item
    this->items[index].text = strdup(text);
    this->items[index].data = data;
    this->item_count++;
    this->needs_redraw = true;
}

bool Panel_removeItem(Panel* this, int index) {
    if (!this || index < 0 || index >= (int)this->item_count) {
        return false;
    }
    
    free(this->items[index].text);
    
    // Shift items left
    memmove(&this->items[index], &this->items[index + 1], 
            (this->item_count - index - 1) * sizeof(PanelItem));
    
    this->item_count--;
    
    // Adjust selection if needed
    if (this->selected >= (int)this->item_count && this->item_count > 0) {
        this->selected = this->item_count - 1;
    }
    
    this->needs_redraw = true;
    return true;
}

void Panel_clear(Panel* this) {
    if (!this) return;
    
    for (size_t i = 0; i < this->item_count; i++) {
        // 1. Free User Data if callback exists
        if (this->cleanup_item && this->items[i].data) {
            this->cleanup_item(this->items[i].data);
        }
        // 2. Free Text
        free(this->items[i].text);
    }
    
    this->item_count = 0;
    this->selected = 0;
    this->scroll_v = 0;
    this->scroll_h = 0;
    this->needs_redraw = true;
}

int Panel_getItemCount(const Panel* this) {
    return this ? (int)this->item_count : 0;
}

PanelItem* Panel_getItem(Panel* this, int index) {
    if (!this || index < 0 || index >= (int)this->item_count) {
        return NULL;
    }
    return &this->items[index];
}

PanelItem* Panel_getSelected(Panel* this) {
    if (!this || this->item_count == 0) {
        return NULL;
    }
    return &this->items[this->selected];
}

int Panel_getSelectedIndex(const Panel* this) {
    if (!this || this->item_count == 0) {
        return -1;
    }
    return this->selected;
}

void Panel_setSelected(Panel* this, int index) {
    if (!this) {
        return;
    }
    
    int size = (int)this->item_count;
    if (size == 0) {
        this->selected = 0;
        return;
    }
    
    // Clamp to valid range
    this->selected = CLAMP(index, 0, size - 1);
    this->needs_redraw = true;
}

static void drawDefaultItem(Panel* this, int index, int y, int x, int w, bool selected) {
    PanelItem* item = &this->items[index];
    
    // Apply selection color if selected
    if (selected) {
        attron(Terminal_colors[this->has_focus ? TEXT_SELECTED : TEXT_DIM]);
    } else {
        attron(Terminal_colors[TEXT_NORMAL]);
    }
    
    // Clear line
    mvhline(y, x, ' ', w);
    
    // Draw text (truncated to width, accounting for scroll)
    int text_len = strlen(item->text);
    if (this->scroll_h < text_len) {
        int display_len = MIN(text_len - this->scroll_h, w);
        mvprintw(y, x, "%.*s", display_len, item->text + this->scroll_h);
    }
    
    if (selected) {
        attroff(Terminal_colors[this->has_focus ? TEXT_SELECTED : TEXT_DIM]);
    } else {
        attroff(Terminal_colors[TEXT_NORMAL]);
    }
}

void Panel_draw(Panel* this, bool force_redraw) {
    if (!this) return;
    
    if (!this->needs_redraw && !force_redraw) return;
    
    int y_pos = this->y;
    int available_height = this->h;
    
    // 1. Draw Header
    if (this->header) {
        attron(Terminal_colors[this->has_focus ? PANEL_HEADER : TEXT_DIM]);
        mvhline(y_pos, this->x, ' ', this->w); // Clear Header Background
        mvprintw(y_pos, this->x, "%.*s", this->w - 1, this->header);
        attroff(Terminal_colors[this->has_focus ? PANEL_HEADER : TEXT_DIM]);
        y_pos++;
        available_height--;
    }
    
    // 2. Draw Top Border (Horizontal)
    attron(Terminal_colors[this->has_focus ? PANEL_BORDER_ACTIVE : PANEL_BORDER]);
    mvhline(y_pos, this->x, ACS_HLINE, this->w - 1); // -1 to save spot for corner/intersection
    attroff(Terminal_colors[this->has_focus ? PANEL_BORDER_ACTIVE : PANEL_BORDER]);
    y_pos++;
    available_height--;
    
    // 3. Calculate Scroll & Visible Range
    int size = (int)this->item_count;
    
    // Calculate how many full items fit in the height
    int visible_items = available_height / this->item_height;
    if (visible_items < 1) visible_items = 1;

    // Adjust scroll
    if (this->selected < this->scroll_v) {
        this->scroll_v = this->selected;
    } else if (this->selected >= this->scroll_v + visible_items) {
        this->scroll_v = this->selected - visible_items + 1;
    }
    this->scroll_v = CLAMP(this->scroll_v, 0, MAX(0, size - visible_items));
    
    int first = this->scroll_v;
    int last = MIN(first + visible_items, size);
    
    int content_width = this->w - 2; 

    // 4. Draw Items
    // We loop through ITEMS, not rows
    for (int i = first; i < last; i++) {
        // Calculate the screen Y position for the top of this item
        int relative_index = i - first;
        int item_y = y_pos + (relative_index * this->item_height);
        
        bool is_selected = (i == this->selected);
        
        if (this->draw_item) {
            // Pass the specific item height to the drawer
            // Note: We pass 'this->item_height' as the 'w' param? No, keep 'content_width'
            // The drawer knows the height from the panel or we should update the typedef.
            // For now, the drawer assumes it can draw up to panel->item_height.
            this->draw_item(this, i, item_y, this->x, content_width, is_selected);
        } else {
            drawDefaultItem(this, i, item_y, this->x, content_width, is_selected);
        }
    }
    
    // Fill remaining empty space at bottom
    int drawn_height = (last - first) * this->item_height;
    for (int y = drawn_height; y < available_height; y++) {
         mvhline(y_pos + y, this->x, ' ', this->w - 1);
         // Draw Right Separator
         attron(Terminal_colors[PANEL_BORDER]);
         mvaddch(y_pos + y, this->x + this->w - 1, ACS_VLINE); 
         attroff(Terminal_colors[PANEL_BORDER]);
    }
    
    // 5. Draw Scrollbar Indicator (if needed)
    if (size > available_height) {
        int scrollbar_pos = (this->scroll_v * (available_height - 1)) / MAX(1, size - 1);
        int scrollbar_y = y_pos + scrollbar_pos;
        
        attron(Terminal_colors[PANEL_BORDER]);
        // Draw scrollbar just inside the vertical separator
        mvaddch(scrollbar_y, this->x + this->w - 2, ACS_CKBOARD); 
        attroff(Terminal_colors[PANEL_BORDER]);
    }
    
    this->needs_redraw = false;
}

bool Panel_onKey(Panel* this, int key) {
    if (!this) {
        return false;
    }
    
    // Try custom handler first
    if (this->event_handler) {
        HandlerResult result = this->event_handler(this, key);
        if (result & HANDLED) {
            return true;
        }
    }
    
    int size = (int)this->item_count;
    if (size == 0) {
        return false;
    }
    
    int old_selected = this->selected;
    int old_scroll = this->scroll_v;
    int available_height = this->h - (this->header ? 2 : 1); // Minus header and border
    
    switch (key) {
        case KEY_UP:
        case 'k':
            this->selected--;
            break;
            
        case KEY_DOWN:
        case 'j':
            this->selected++;
            break;
            
        case KEY_PPAGE:  // Page Up
            this->selected -= available_height;
            this->scroll_v -= available_height;
            break;
            
        case KEY_NPAGE:  // Page Down
            this->selected += available_height;
            this->scroll_v += available_height;
            break;
            
        case KEY_HOME:
        case 'g':
            this->selected = 0;
            break;
            
        case KEY_END:
        case 'G':
            this->selected = size - 1;
            break;
            
        case KEY_LEFT:
        case 'h':
            if (this->scroll_h > 0) {
                this->scroll_h -= 5;
                if (this->scroll_h < 0) {
                    this->scroll_h = 0;
                }
            }
            break;
            
        case KEY_RIGHT:
        case 'l':
            this->scroll_h += 5;
            break;
            
        default:
            return false;
    }
    
    // Clamp selection
    this->selected = CLAMP(this->selected, 0, size - 1);
    
    // Clamp scroll
    this->scroll_v = CLAMP(this->scroll_v, 0, MAX(0, size - available_height));
    
    // Mark for redraw if changed
    if (this->selected != old_selected || this->scroll_v != old_scroll) {
        this->needs_redraw = true;
    }
    
    return true;
}

void Panel_setNeedsRedraw(Panel* this) {
    if (this) {
        this->needs_redraw = true;
    }
}

void Panel_setFocus(Panel* this, bool focus) {
    if (this) {
        this->has_focus = focus;
        this->needs_redraw = true;
    }
}

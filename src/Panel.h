#ifndef EXPML_PANEL_H
#define EXPML_PANEL_H

#include <stdbool.h>
#include <stddef.h>
#include "ScreenManager.h" // Include this to get HandlerResult definition

/*
 * Panel - Scrollable list widget with borders
 */

// Forward declare Panel for function pointer types
typedef struct Panel_ Panel;

/*
 * Panel_EventHandler
 */
typedef HandlerResult (*Panel_EventHandler)(Panel*, int key);

/*
 * Panel_DrawItem
 */
typedef void (*Panel_DrawItem)(Panel* panel, int index, int y, int x, int w, bool selected);

/*
 * PanelItem
 */
typedef struct PanelItem_ {
   char* text;
   void* data;
} PanelItem;

/*
 * Panel Struct
 */
struct Panel_ {
   int x, y;
   int w, h;
   char* header;
   
   PanelItem* items;
   size_t item_count;
   size_t item_capacity;
   
   int selected;
   int scroll_v;
   int scroll_h;

   int item_height;
   bool needs_redraw;
   bool has_focus;      
   
   Panel_EventHandler event_handler;
   Panel_DrawItem draw_item;
   void* user_data;
};

/*
 * API Functions
 */
Panel* Panel_new(int x, int y, int w, int h, const char* header);
void Panel_delete(Panel* this);

// Configuration
void Panel_setEventHandler(Panel* this, Panel_EventHandler handler);
void Panel_setDrawItem(Panel* this, Panel_DrawItem draw_item);
void Panel_setUserData(Panel* this, void* user_data);
void* Panel_getUserData(Panel* this);
void Panel_setHeader(Panel* this, const char* header);

// Layout
void Panel_move(Panel* this, int x, int y);
void Panel_resize(Panel* this, int w, int h);

// Items
int Panel_addItem(Panel* this, const char* text, void* data);
void Panel_insertItem(Panel* this, int index, const char* text, void* data);
bool Panel_removeItem(Panel* this, int index);
void Panel_clear(Panel* this);

// Accessors
int Panel_getItemCount(const Panel* this);
PanelItem* Panel_getItem(Panel* this, int index);
PanelItem* Panel_getSelected(Panel* this);
int Panel_getSelectedIndex(const Panel* this);
void Panel_setSelected(Panel* this, int index);

// Drawing & Input
void Panel_draw(Panel* this, bool force_redraw);
bool Panel_onKey(Panel* this, int key);
void Panel_setNeedsRedraw(Panel* this);

// Focus management (Missing piece we added)
void Panel_setFocus(Panel* this, bool focus);

// Set the height (in rows) of a single item. Default is 1.
void Panel_setItemHeight(Panel* this, int h);

#endif

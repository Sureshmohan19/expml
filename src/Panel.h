#ifndef EXPML_PANEL_H
#define EXPML_PANEL_H

#include "ScreenManager.h" 

#include <stdbool.h>
#include <stddef.h>

typedef struct Panel_ Panel;
typedef HandlerResult (*Panel_EventHandler)(Panel*, int key);
typedef void (*Panel_DrawItem)(Panel* panel, int index, int y, int x, int w, bool selected);
typedef void (*Panel_ItemCleanup)(void* data);

typedef struct PanelItem_ {
   char* text;
   void* data;
} PanelItem;

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
   bool draw_right_separator; 
   Panel_EventHandler event_handler;
   Panel_DrawItem draw_item;
   Panel_ItemCleanup cleanup_item;
   void* user_data;
};

Panel* Panel_new(int x, int y, int w, int h, const char* header);
void Panel_delete(Panel* this);
void Panel_setEventHandler(Panel* this, Panel_EventHandler handler);
void Panel_setDrawItem(Panel* this, Panel_DrawItem draw_item);
void Panel_setUserData(Panel* this, void* user_data);
void* Panel_getUserData(Panel* this);
void Panel_setHeader(Panel* this, const char* header);
void Panel_move(Panel* this, int x, int y);
void Panel_resize(Panel* this, int w, int h);
int Panel_addItem(Panel* this, const char* text, void* data);
void Panel_insertItem(Panel* this, int index, const char* text, void* data);
bool Panel_removeItem(Panel* this, int index);
void Panel_clear(Panel* this);
int Panel_getItemCount(const Panel* this);
PanelItem* Panel_getItem(Panel* this, int index);
PanelItem* Panel_getSelected(Panel* this);
int Panel_getSelectedIndex(const Panel* this);
void Panel_setSelected(Panel* this, int index);
void Panel_draw(Panel* this, bool force_redraw);
bool Panel_onKey(Panel* this, int key);
void Panel_setNeedsRedraw(Panel* this);
void Panel_setDrawRightSeparator(Panel* this, bool draw);
void Panel_setFocus(Panel* this, bool focus);
void Panel_setItemHeight(Panel* this, int h);
void Panel_setCleanupCallback(Panel* this, Panel_ItemCleanup callback);

#endif

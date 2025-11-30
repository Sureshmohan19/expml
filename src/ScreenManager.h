#ifndef EXPML_SCREENMANAGER_H
#define EXPML_SCREENMANAGER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Panel_ Panel;
typedef struct FunctionBar_ FunctionBar;
typedef struct ScreenManager_ ScreenManager;

typedef enum HandlerResult_ {
   HANDLED      = 0x01,  // Event was handled
   IGNORED      = 0x02,  // Event was not handled
   BREAK_LOOP   = 0x04,  // Exit the main loop
   REFRESH      = 0x08,  // Trigger data refresh
   REDRAW       = 0x10,  // Force screen redraw
   RESIZE       = 0x40,  // Recalculate layout
} HandlerResult;

ScreenManager* ScreenManager_new(const char* header_text, double refresh_interval);
void ScreenManager_delete(ScreenManager* this);
void ScreenManager_addPanel(ScreenManager* this, Panel* panel, int width);
Panel* ScreenManager_removePanel(ScreenManager* this, size_t index);
void ScreenManager_resize(ScreenManager* this);
bool ScreenManager_setFocus(ScreenManager* this, size_t index);
Panel* ScreenManager_getFocused(ScreenManager* this);
void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* bar);
void ScreenManager_setHeaderText(ScreenManager* this, const char* text);
int ScreenManager_run(ScreenManager* this);
void ScreenManager_forceRedraw(ScreenManager* this);
size_t ScreenManager_getPanelCount(const ScreenManager* this);
Panel* ScreenManager_getPanel(const ScreenManager* this, size_t index);
typedef void (*ScreenManager_OnRefresh)(void* userdata);
void ScreenManager_setRefreshCallback(ScreenManager* this, ScreenManager_OnRefresh callback, void* userdata);
void ScreenManager_setStartTime(ScreenManager* this, const char* time);
void ScreenManager_setEndTime(ScreenManager* this, const char* time);
void ScreenManager_setHeaderStatus(ScreenManager* this, const char* status);
void ScreenManager_setHeaderRuntime(ScreenManager* this, double runtime);

#endif

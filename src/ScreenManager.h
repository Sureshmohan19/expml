#ifndef EXPML_SCREENMANAGER_H
#define EXPML_SCREENMANAGER_H

#include <stdbool.h>
#include <stddef.h>

/*
 * ScreenManager - Main TUI coordinator
 */

// Forward declarations
typedef struct Panel_ Panel;
typedef struct FunctionBar_ FunctionBar;

/*
 * HandlerResult - Return values from panel event handlers
 */
typedef enum HandlerResult_ {
   HANDLED      = 0x01,  // Event was handled
   IGNORED      = 0x02,  // Event was not handled
   BREAK_LOOP   = 0x04,  // Exit the main loop
   REFRESH      = 0x08,  // Trigger data refresh
   REDRAW       = 0x10,  // Force screen redraw
   RESIZE       = 0x40,  // Recalculate layout
} HandlerResult;

/*
 * Opaque struct definition. 
 * The actual fields are defined in ScreenManager.c
 */
typedef struct ScreenManager_ ScreenManager;

/*
 * Create new ScreenManager
 */
ScreenManager* ScreenManager_new(const char* header_text, double refresh_interval);

/*
 * Delete ScreenManager
 */
void ScreenManager_delete(ScreenManager* this);

/*
 * Add a panel to the screen
 * width: specific column width, or 0 to auto-stretch
 */
void ScreenManager_addPanel(ScreenManager* this, Panel* panel, int width);

/*
 * Remove a panel
 */
Panel* ScreenManager_removePanel(ScreenManager* this, size_t index);

/*
 * Recalculate layout
 */
void ScreenManager_resize(ScreenManager* this);

/*
 * Set focus
 */
bool ScreenManager_setFocus(ScreenManager* this, size_t index);

/*
 * Get focused panel
 */
Panel* ScreenManager_getFocused(ScreenManager* this);

/*
 * Set function bar
 */
void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* bar);

/*
 * Update header text
 */
void ScreenManager_setHeaderText(ScreenManager* this, const char* text);

/*
 * Main event loop
 */
int ScreenManager_run(ScreenManager* this);

/*
 * Force redraw
 */
void ScreenManager_forceRedraw(ScreenManager* this);

/*
 * Get panel count
 */
size_t ScreenManager_getPanelCount(const ScreenManager* this);

/*
 * Get panel by index
 */
Panel* ScreenManager_getPanel(const ScreenManager* this, size_t index);

// Add this typedef before ScreenManager struct or inside the header
typedef void (*ScreenManager_OnRefresh)(void* userdata);

// Add this function declaration
void ScreenManager_setRefreshCallback(ScreenManager* this, ScreenManager_OnRefresh callback, void* userdata);

#endif

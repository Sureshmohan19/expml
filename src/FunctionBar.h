#ifndef EXPML_FUNCTIONBAR_H
#define EXPML_FUNCTIONBAR_H

/*
 * FunctionBar - Bottom status bar
 * Shows context info (left) and available keys (right)
 */

typedef struct FunctionBar_ FunctionBar;

/*
 * Create a new function bar
 * keys: Array of keys (e.g. {"F1", "F10", NULL})
 * labels: Array of labels (e.g. {"Help", "Quit", NULL})
 * 
 * Returns: New FunctionBar instance
 */
FunctionBar* FunctionBar_new(const char* const* keys, const char* const* labels);

/*
 * Free resources
 */
void FunctionBar_delete(FunctionBar* this);

/*
 * Set the context text (left side of bar)
 * Supports printf-style formatting
 * Example: FunctionBar_setContext(bar, "Run: %s | Loss: %.4f", run_name, loss);
 */
void FunctionBar_setContext(FunctionBar* this, const char* fmt, ...);

/*
 * Draw the bar at the bottom of the screen
 * width: Width of the screen (usually COLS)
 */
void FunctionBar_draw(const FunctionBar* this, int width);

#endif

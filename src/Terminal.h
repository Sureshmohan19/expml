#ifndef EXPML_TERMINAL_H
#define EXPML_TERMINAL_H

#include <stdbool.h>

/*
 * Terminal - Terminal/Curses management for expml TUI
 * Handles ncurses initialization, color schemes, and input
 */

// Color scheme enumeration
typedef enum ColorScheme_ {
   COLORSCHEME_DARK,      // Dark theme (default, based on provided image)
   COLORSCHEME_MONOCHROME, // Fallback for terminals without color support
   LAST_COLORSCHEME
} ColorScheme;

// Color elements for different UI components
typedef enum ColorElement_ {
   // Base colors
   RESET_COLOR,
   DEFAULT_COLOR,
   
   // Text colors
   TEXT_NORMAL,           // Main text color (light gray/white)
   TEXT_DIM,              // Dimmed/secondary text (darker gray)
   TEXT_BRIGHT,           // Bright/emphasized text (white)
   TEXT_SELECTED,         // Selected item text (highlighted)
   
   // Status colors
   COLOR_ERROR,           // Error messages (red/pink)
   COLOR_SUCCESS,         // Success messages (green)
   COLOR_WARNING,         // Warning messages (yellow/orange)
   COLOR_INFO,            // Info/accent (cyan/blue)
   
   // UI elements
   PANEL_HEADER,          // Panel headers (Run Overview, Metrics, etc.)
   PANEL_BORDER,          // Panel borders
   PANEL_BORDER_ACTIVE,   // Active/focused panel border
   PANEL_BACKGROUND,      // Panel background
   
   // Data visualization
   GRAPH_LINE,            // Graph lines (orange/yellow from image)
   GRAPH_DOTS,            // Graph data points (dotted lines)
   GRAPH_AXIS,            // Graph axes and labels
   METRIC_VALUE,          // Metric values
   METRIC_LABEL,          // Metric labels
   
   // Special elements
   STATUS_BAR,            // Bottom status bar
   HELP_TEXT,             // Help text at bottom
   
   LAST_COLORELEMENT
} ColorElement;

// Key definitions (extended keys for TUI navigation)
// TODO: Add custom key mappings as needed
#define KEY_CTRL(x) ((x) & 0x1f)

// Global state
extern ColorScheme Terminal_colorScheme;
extern const int* Terminal_colors;

/*
 * Initialize terminal and ncurses
 * Sets up color pairs, input mode, and screen
 * TODO: Implement in Terminal.c
 */
void Terminal_init(bool allowUnicode);

/*
 * Cleanup and restore terminal to normal mode
 * TODO: Implement in Terminal.c
 */
void Terminal_done(void);

/*
 * Read a key from input
 * Returns: key code (ncurses KEY_* or character)
 * TODO: Implement in Terminal.c
 */
int Terminal_readKey(void);

/*
 * Set the color scheme
 * scheme: COLORSCHEME_DARK or COLORSCHEME_MONOCHROME
 * TODO: Implement in Terminal.c with color pair initialization
 */
void Terminal_setColors(ColorScheme scheme);

/*
 * Fatal error handler - print error and exit
 * TODO: Implement in Terminal.c
 */
void Terminal_fatalError(const char* message) __attribute__((noreturn));

#endif

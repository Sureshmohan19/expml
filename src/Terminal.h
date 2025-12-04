#ifndef EXPML_TERMINAL_H
#define EXPML_TERMINAL_H

#include <stdbool.h>

#define CHART_PALETTE_SIZE 10

typedef enum ColorScheme_ {
   COLORSCHEME_DARK,
   COLORSCHEME_MONOCHROME,
   LAST_COLORSCHEME
} ColorScheme;

typedef enum ColorElement_ {
   RESET_COLOR,
   DEFAULT_COLOR,
   TEXT_NORMAL,
   TEXT_DIM,
   TEXT_BRIGHT,
   TEXT_SELECTED,
   COLOR_ERROR,
   COLOR_SUCCESS,
   COLOR_WARNING,
   COLOR_INFO,
   PANEL_HEADER,
   PANEL_BORDER,
   PANEL_HEADER_DIM,
   PANEL_BORDER_ACTIVE,
   PANEL_BACKGROUND,

   // mark for deprecation
   GRAPH_LINE,
   GRAPH_DOTS,

   GRAPH_AXIS,
   METRIC_VALUE,
   METRIC_LABEL,
   STATUS_BAR,
   HELP_TEXT,
   
   // --- SUNSET PALETTE (New) ---
   CHART_COLOR_1, // Ink Black
   CHART_COLOR_2, // Dark Teal
   CHART_COLOR_3, // Dark Cyan
   CHART_COLOR_4, // Pearl Aqua
   CHART_COLOR_5, // Wheat
   CHART_COLOR_6, // Golden Orange
   CHART_COLOR_7, // Burnt Caramel
   CHART_COLOR_8, // Rusty Spice
   CHART_COLOR_9, // Oxidized Iron
   CHART_COLOR_10,// Brown Red

   LAST_COLORELEMENT
} ColorElement;

#define KEY_CTRL(x) ((x) & 0x1f)

extern ColorScheme Terminal_colorScheme;
extern const int* Terminal_colors;

void Terminal_init(bool allowUnicode);
void Terminal_done(void);
int Terminal_readKey(void);
void Terminal_setColors(ColorScheme scheme);
void Terminal_fatalError(const char* message) __attribute__((noreturn));
void Terminal_resetColors(void);

#endif
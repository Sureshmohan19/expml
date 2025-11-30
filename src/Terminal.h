#ifndef EXPML_TERMINAL_H
#define EXPML_TERMINAL_H

#include <stdbool.h>

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
   GRAPH_LINE,
   GRAPH_DOTS,
   GRAPH_AXIS,
   METRIC_VALUE,
   METRIC_LABEL,
   STATUS_BAR,
   HELP_TEXT,
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

#endif
#define _POSIX_C_SOURCE 200809L
#include "Terminal.h"
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Color pair index calculation
#define ColorIndex(fg, bg) (((fg) * 8) + (bg))
#define ColorPair(fg, bg) COLOR_PAIR(ColorIndex(fg, bg))

// Basic ncurses colors
#define Black   COLOR_BLACK
#define Red     COLOR_RED
#define Green   COLOR_GREEN
#define Yellow  COLOR_YELLOW
#define Blue    COLOR_BLUE
#define Magenta COLOR_MAGENTA
#define Cyan    COLOR_CYAN
#define White   COLOR_WHITE

// Global state
ColorScheme Terminal_colorScheme = COLORSCHEME_DARK;
const int* Terminal_colors = NULL;

// --- MINIMALIST BLACK/WHITE/GREY WITH DARK ORANGE ACCENTS ---
static int Terminal_colorSchemes[LAST_COLORSCHEME][LAST_COLORELEMENT] = {
   [COLORSCHEME_DARK] = {
      [RESET_COLOR] = ColorPair(White, Black),
      [DEFAULT_COLOR] = ColorPair(White, Black),
      
      // Text colors - Pure monochrome hierarchy
      [TEXT_NORMAL] = ColorPair(White, Black),              // Bright white for primary text
      [TEXT_DIM] = A_DIM | ColorPair(White, Black),         // Dimmed white for secondary
      [TEXT_BRIGHT] = A_BOLD | ColorPair(White, Black),     // Bold white for emphasis
      
      // Selection - Subtle grey highlight
      [TEXT_SELECTED] = A_REVERSE | ColorPair(White, Black), // White on black (grey appearance)
      
      // Status colors - Minimal color use
      [COLOR_ERROR] = A_BOLD | ColorPair(Red, Black),       // Keep red for errors (critical)
      [COLOR_SUCCESS] = ColorPair(White, Black),            // White for success (minimalist)
      [COLOR_WARNING] = ColorPair(Yellow, Black),           // Yellow for warnings
      [COLOR_INFO] = ColorPair(White, Black),               // White for info
      
      // UI elements - Dark orange accents for headings only
      [PANEL_HEADER] = A_BOLD | ColorPair(Yellow, Black),   // Bold Yellow (renders as orange) for headings
      [PANEL_BORDER] = A_DIM | ColorPair(White, Black),     // Very subtle grey borders
      [PANEL_BORDER_ACTIVE] = ColorPair(White, Black),      // Bright white for active
      [PANEL_BACKGROUND] = ColorPair(White, Black),
      
      // Data visualization - Monochrome with subtle shading
      [GRAPH_LINE] = ColorPair(White, Black),               // White lines
      [GRAPH_DOTS] = A_DIM | ColorPair(White, Black),       // Grey dots
      [GRAPH_AXIS] = A_DIM | ColorPair(White, Black),       // Grey axis
      [METRIC_VALUE] = A_BOLD | ColorPair(White, Black),    // Bold values
      [METRIC_LABEL] = A_DIM | ColorPair(White, Black),     // Dim labels
      
      // Special elements
      [STATUS_BAR] = A_REVERSE | ColorPair(White, Black),   // Inverted white/black
      [HELP_TEXT] = A_DIM | ColorPair(White, Black),        // Subtle grey help text
   },
};

// Signal handlers
static struct sigaction old_sig_handler[32];

static void Terminal_handleSIGTERM(int sgn) {
   Terminal_done();
   _exit(0);
}

static void Terminal_installSignalHandlers(void) {
   struct sigaction act;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_handler = Terminal_handleSIGTERM;
   
   sigaction(SIGINT, &act, &old_sig_handler[SIGINT]);
   sigaction(SIGTERM, &act, &old_sig_handler[SIGTERM]);
   sigaction(SIGQUIT, &act, &old_sig_handler[SIGQUIT]);
}

void Terminal_init(bool allowUnicode) {
   if (allowUnicode) {
       setlocale(LC_ALL, ""); 
   }

   initscr();
   
   noecho();              
   cbreak();              
   nodelay(stdscr, FALSE); 
   keypad(stdscr, TRUE);  
   curs_set(0);           
   
   if (has_colors()) {
      start_color();
      use_default_colors(); // Allows transparency (-1)
      
      // Initialize all color pairs
      for (int fg = 0; fg < 8; fg++) {
         for (int bg = 0; bg < 8; bg++) {
            int actual_bg = (bg == Black) ? -1 : bg;
            init_pair(ColorIndex(fg, bg), fg, actual_bg);
         }
      }
   } else {
      Terminal_colorScheme = COLORSCHEME_MONOCHROME;
   }
   
   Terminal_setColors(Terminal_colorScheme);
   Terminal_installSignalHandlers();
}

void Terminal_done(void) {
   if (Terminal_colors) {
      int resetColor = Terminal_colors[RESET_COLOR];
      attron(resetColor);
      mvhline(LINES - 1, 0, ' ', COLS);
      attroff(resetColor);
      refresh();
   }
   curs_set(1);
   endwin();
}

int Terminal_readKey(void) {
   return getch();
}

void Terminal_setColors(ColorScheme scheme) {
   Terminal_colorScheme = scheme;
   Terminal_colors = Terminal_colorSchemes[scheme];
}

void Terminal_fatalError(const char* message) {
   Terminal_done();
   fprintf(stderr, "Fatal error: %s\n", message);
   exit(1);
}

#include "Terminal.h"

#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Color pair index calculation (similar to htop's ColorIndex)
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

// Color scheme definitions
static int Terminal_colorSchemes[LAST_COLORSCHEME][LAST_COLORELEMENT] = {
   [COLORSCHEME_DARK] = {
      [RESET_COLOR] = ColorPair(White, Black),
      [DEFAULT_COLOR] = ColorPair(White, Black),
      
      // Text colors (based on the dark theme image)
      [TEXT_NORMAL] = ColorPair(White, Black),           // Light gray/white text
      [TEXT_DIM] = ColorPair(Cyan, Black),               // Dimmed secondary text
      [TEXT_BRIGHT] = A_BOLD | ColorPair(White, Black),  // Bright emphasized text
      [TEXT_SELECTED] = ColorPair(Black, Cyan),          // Selected item
      
      // Status colors
      [COLOR_ERROR] = A_BOLD | ColorPair(Red, Black),    // Error messages
      [COLOR_SUCCESS] = ColorPair(Green, Black),         // Success messages
      [COLOR_WARNING] = ColorPair(Yellow, Black),        // Warning messages
      [COLOR_INFO] = ColorPair(Cyan, Black),             // Info/accent
      
      // UI elements
      [PANEL_HEADER] = A_BOLD | ColorPair(Cyan, Black),  // Panel headers
      [PANEL_BORDER] = ColorPair(Cyan, Black),           // Panel borders
      [PANEL_BORDER_ACTIVE] = A_BOLD | ColorPair(Green, Black), // Active border
      [PANEL_BACKGROUND] = ColorPair(White, Black),      // Panel background
      
      // Data visualization (orange/yellow from charts)
      [GRAPH_LINE] = ColorPair(Yellow, Black),           // Graph lines
      [GRAPH_DOTS] = ColorPair(Yellow, Black),           // Graph dots
      [GRAPH_AXIS] = ColorPair(Cyan, Black),             // Graph axes
      [METRIC_VALUE] = A_BOLD | ColorPair(White, Black), // Metric values
      [METRIC_LABEL] = ColorPair(Cyan, Black),           // Metric labels
      
      // Special elements
      [STATUS_BAR] = ColorPair(Black, Cyan),             // Bottom status bar
      [HELP_TEXT] = ColorPair(White, Black),             // Help text
   },
   [COLORSCHEME_MONOCHROME] = {
      [RESET_COLOR] = A_NORMAL,
      [DEFAULT_COLOR] = A_NORMAL,
      
      // Text - use attributes instead of colors
      [TEXT_NORMAL] = A_NORMAL,
      [TEXT_DIM] = A_DIM,
      [TEXT_BRIGHT] = A_BOLD,
      [TEXT_SELECTED] = A_REVERSE,
      
      // Status
      [COLOR_ERROR] = A_BOLD,
      [COLOR_SUCCESS] = A_NORMAL,
      [COLOR_WARNING] = A_BOLD,
      [COLOR_INFO] = A_NORMAL,
      
      // UI
      [PANEL_HEADER] = A_REVERSE,
      [PANEL_BORDER] = A_NORMAL,
      [PANEL_BORDER_ACTIVE] = A_BOLD,
      [PANEL_BACKGROUND] = A_NORMAL,
      
      // Data
      [GRAPH_LINE] = A_NORMAL,
      [GRAPH_DOTS] = A_NORMAL,
      [GRAPH_AXIS] = A_NORMAL,
      [METRIC_VALUE] = A_BOLD,
      [METRIC_LABEL] = A_NORMAL,
      
      // Special
      [STATUS_BAR] = A_REVERSE,
      [HELP_TEXT] = A_NORMAL,
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
   
   // Handle termination signals gracefully
   sigaction(SIGINT, &act, &old_sig_handler[SIGINT]);
   sigaction(SIGTERM, &act, &old_sig_handler[SIGTERM]);
   sigaction(SIGQUIT, &act, &old_sig_handler[SIGQUIT]);
}

void Terminal_init(bool allowUnicode) {
   // 1. ENABLE UNICODE/UTF-8 (Must be before initscr)
   if (allowUnicode) {
       setlocale(LC_ALL, ""); 
   }

   // Initialize ncurses
   initscr();
   
   // Setup terminal mode
   noecho();              // Don't echo input
   cbreak();              // Disable line buffering
   nodelay(stdscr, FALSE); // Blocking reads
   keypad(stdscr, TRUE);  // Enable function keys and arrow keys
   curs_set(0);           // Hide cursor
   
   // Initialize colors if supported
   if (has_colors()) {
      start_color();
      use_default_colors();
      
      // Initialize all color pairs
      for (int fg = 0; fg < 8; fg++) {
         for (int bg = 0; bg < 8; bg++) {
            // Use -1 for default background (transparent)
            int actual_bg = (bg == Black) ? -1 : bg;
            init_pair(ColorIndex(fg, bg), fg, actual_bg);
         }
      }
   } else {
      // No color support - use monochrome
      Terminal_colorScheme = COLORSCHEME_MONOCHROME;
   }
   
   // Set default color scheme
   Terminal_setColors(Terminal_colorScheme);
   
   // Install signal handlers
   Terminal_installSignalHandlers();
   
   // TODO: Handle unicode support when needed
   (void)allowUnicode;
}

void Terminal_done(void) {
   // Clear and refresh before exit
   if (Terminal_colors) {
      int resetColor = Terminal_colors[RESET_COLOR];
      attron(resetColor);
      mvhline(LINES - 1, 0, ' ', COLS);
      attroff(resetColor);
      refresh();
   }
   
   // Show cursor again
   curs_set(1);
   
   // End ncurses mode
   endwin();
}

int Terminal_readKey(void) {
   // Read a key from input
   return getch();
}

void Terminal_setColors(ColorScheme scheme) {
   Terminal_colorScheme = scheme;
   Terminal_colors = Terminal_colorSchemes[scheme];
}

void Terminal_fatalError(const char* message) {
   // Clean up terminal first
   Terminal_done();
   
   // Print error and exit
   fprintf(stderr, "Fatal error: %s\n", message);
   exit(1);
}

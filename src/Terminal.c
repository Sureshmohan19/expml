#define _POSIX_C_SOURCE 200809L

#include "Terminal.h"
#include <stdbool.h>
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ColorIndex(fg, bg) (((7-(fg)) * 8) + (bg))
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

#define PAIR_GRAPH_LINE  10
#define PAIR_HEADER      11
#define PAIR_HEADER_DIM  12
#define COLOR_HEX_HEADING     20  // #F3C62F
#define COLOR_HEX_SIDE        21  // #49D3F2
#define COLOR_HEX_CHART_UP    22  // #B7E9C1
#define COLOR_HEX_CHART_DOWN  23  // #AE7EF2

ColorScheme Terminal_colorScheme = COLORSCHEME_DARK;
const int* Terminal_colors = NULL;
static void Terminal_initCustomColors(void);

static int Terminal_colorSchemes[LAST_COLORSCHEME][LAST_COLORELEMENT] = {
   [COLORSCHEME_DARK] = {
      [RESET_COLOR]           = ColorPair(White, Black),
      [DEFAULT_COLOR]         = ColorPair(White, Black),
      [TEXT_NORMAL]           = ColorPair(White, Black),
      [TEXT_DIM]              = A_DIM | ColorPair(White, Black),
      [TEXT_BRIGHT]           = A_BOLD | ColorPair(White, Black),
      [TEXT_SELECTED]         = A_REVERSE | ColorPair(White, Black),
      [COLOR_ERROR]           = A_BOLD | ColorPair(Red, Black),
      [COLOR_SUCCESS]         = ColorPair(White, Black),
      [COLOR_WARNING]         = ColorPair(Yellow, Black),
      [COLOR_INFO]            = ColorPair(White, Black),
      [PANEL_HEADER]          = A_BOLD | ColorPair(Green, Black),
      [PANEL_HEADER_DIM]      = A_DIM | ColorPair(Green, Black),
      [PANEL_BORDER]          = A_DIM | ColorPair(White, Black),
      [PANEL_BORDER_ACTIVE]   = ColorPair(White, Black),
      [PANEL_BACKGROUND]      = ColorPair(White, Black),
      [GRAPH_LINE]            = ColorPair(Cyan, Black),
      [GRAPH_DOTS]            = A_DIM | ColorPair(Cyan, Black),
      [GRAPH_AXIS]            = A_DIM | ColorPair(White, Black),
      [METRIC_VALUE]          = A_BOLD | ColorPair(White, Black),
      [METRIC_LABEL]          = A_DIM | ColorPair(White, Red),
      [STATUS_BAR]            = ColorPair(Black, White),  // Black text on white background
      [HELP_TEXT]             = A_DIM | ColorPair(White, Black),
   },
};

static struct sigaction old_sig_handler[32];
static void Terminal_handleSIGTERM(int sgn) { Terminal_done(); _exit(0); }
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
   if (allowUnicode) { setlocale(LC_ALL, ""); }
   
   initscr();
   noecho();              
   cbreak();              
   nodelay(stdscr, FALSE); 
   keypad(stdscr, TRUE);  
   curs_set(0);           

   // --- DISABLE MOUSE INTERACTION ---
   // Passing 0 tells ncurses to ignore all mouse events.
   mousemask(0, NULL);      
   
   if (has_colors()) {
      start_color();
      use_default_colors(); 

      for (int fg = 0; fg < 8; fg++) {
         for (int bg = 0; bg < 8; bg++) {
            int actual_bg = bg;
            init_pair(ColorIndex(fg, bg), fg, actual_bg);
         }
      }
      // DEBUG: Test a specific pair
        init_pair(99, COLOR_WHITE, COLOR_CYAN);
        fprintf(stderr, "Color pairs initialized. COLORS=%d, COLOR_PAIRS=%d\n", 
                COLORS, COLOR_PAIRS);
   } else {
      Terminal_colorScheme = COLORSCHEME_MONOCHROME;
   }
   
   Terminal_initCustomColors();
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
   int ch = getch();
   // Double safety: if a mouse event somehow slips through, ignore it.
   if (ch == KEY_MOUSE) return ERR;
   return ch; 
}

void Terminal_setColors(ColorScheme scheme) {
   Terminal_colorScheme = scheme;
   Terminal_colors = Terminal_colorSchemes[scheme];
}

static void Terminal_initCustomColors(void) {
   if (!has_colors()) { return; }

   short graph_color = COLOR_CYAN;
   short header_color = COLOR_GREEN;

   bool can_set_rgb = can_change_color() && COLORS >= 256 && COLOR_PAIRS > PAIR_HEADER;

   // Debug: Check terminal capabilities
   fprintf(stderr, "can_change_color: %d, COLORS: %d, COLOR_PAIRS: %d\n", 
            can_change_color(), COLORS, COLOR_PAIRS);
   
   if (can_set_rgb) {
      init_color(COLOR_HEX_HEADING, 953, 776, 184);    // #F3C62F
      init_color(COLOR_HEX_SIDE, 286, 827, 949);       // #49D3F2
      init_color(COLOR_HEX_CHART_UP, 718, 914, 757);   // #B7E9C1
      init_color(COLOR_HEX_CHART_DOWN, 682, 494, 949); // #AE7EF2
      graph_color = COLOR_HEX_CHART_UP;
      header_color = COLOR_HEX_HEADING;
   }

   init_pair(PAIR_GRAPH_LINE, graph_color, -1);
   init_pair(PAIR_HEADER, header_color, -1);
   init_pair(PAIR_HEADER_DIM, header_color, -1); 
   int graph_pair_attr = COLOR_PAIR(PAIR_GRAPH_LINE);
   int header_pair_attr = A_BOLD | COLOR_PAIR(PAIR_HEADER);
   int header_dim_attr = A_DIM | COLOR_PAIR(PAIR_HEADER_DIM);

   Terminal_colorSchemes[COLORSCHEME_DARK][GRAPH_LINE] = graph_pair_attr;
   Terminal_colorSchemes[COLORSCHEME_DARK][GRAPH_DOTS] = A_DIM | graph_pair_attr;
   Terminal_colorSchemes[COLORSCHEME_DARK][PANEL_HEADER] = header_pair_attr;
   Terminal_colorSchemes[COLORSCHEME_DARK][PANEL_HEADER_DIM] = header_dim_attr; 
}

void Terminal_fatalError(const char* message) {
   Terminal_done();
   fprintf(stderr, "Fatal error: %s\n", message);
   exit(1);
}

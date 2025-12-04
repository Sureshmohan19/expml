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

// --- Sunset Palette Pairs (IDs 30-39) ---
#define PAIR_CHART_1     30
#define PAIR_CHART_2     31
#define PAIR_CHART_3     32
#define PAIR_CHART_4     33
#define PAIR_CHART_5     34
#define PAIR_CHART_6     35
#define PAIR_CHART_7     36
#define PAIR_CHART_8     37
#define PAIR_CHART_9     38
#define PAIR_CHART_10    39

#define COLOR_HEX_HEADING     20  // #F3C62F
#define COLOR_HEX_SIDE        21  // #49D3F2
#define COLOR_HEX_CHART_UP    22  // #B7E9C1
#define COLOR_HEX_CHART_DOWN  23  // #AE7EF2

// --- Sunset Palette Custom Colors (IDs 50-59) ---
#define C_INK_BLACK     50 // #001219
#define C_DARK_TEAL     51 // #005f73
#define C_DARK_CYAN     52 // #0a9396
#define C_PEARL_AQUA    53 // #94d2bd
#define C_WHEAT         54 // #e9d8a6
#define C_GOLD_ORANGE   55 // #ee9b00
#define C_BURNT_CARAMEL 56 // #ca6702
#define C_RUSTY_SPICE   57 // #bb3e03
#define C_OXIDIZED_IRON 58 // #ae2012
#define C_BROWN_RED     59 // #9b2226

ColorScheme Terminal_colorScheme = COLORSCHEME_DARK;
const int* Terminal_colors = NULL;

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

      // [OLD COLORS]
      [GRAPH_LINE]            = ColorPair(Cyan, Black),
      [GRAPH_DOTS]            = A_DIM | ColorPair(Cyan, Black),
      
      [GRAPH_AXIS]            = A_DIM | ColorPair(White, Black),
      [METRIC_VALUE]          = A_BOLD | ColorPair(White, Black),
      [METRIC_LABEL]          = A_DIM | ColorPair(White, Black),
      [STATUS_BAR]            = ColorPair(Black, White),  // Black text on white background
      [HELP_TEXT]             = A_DIM | ColorPair(White, Black),

      // New Chart Colors (Initialized in initCustomColors)
      [CHART_COLOR_1]         = ColorPair(Blue, Black),
      [CHART_COLOR_2]         = ColorPair(Cyan, Black),
      [CHART_COLOR_3]         = ColorPair(Cyan, Black),
      [CHART_COLOR_4]         = ColorPair(White, Black),
      [CHART_COLOR_5]         = ColorPair(Yellow, Black),
      [CHART_COLOR_6]         = ColorPair(Yellow, Black),
      [CHART_COLOR_7]         = ColorPair(Red, Black),
      [CHART_COLOR_8]         = ColorPair(Red, Black),
      [CHART_COLOR_9]         = ColorPair(Red, Black),
      [CHART_COLOR_10]        = ColorPair(Magenta, Black),
   },
};

static struct sigaction old_sig_handler[32];
static void Terminal_handleSIGTERM(int sgn) { 
    (void)sgn;  // Suppress unused parameter warning
    Terminal_done(); 
    _exit(0); 
}

// We must exit ncurses mode before the kernel stops the process,
// otherwise the terminal state is left messy.
static void Terminal_handleSIGTSTP(int sgn) {
    (void)sgn;
    Terminal_done(); // Exit ncurses mode
    
    // Restore default signal handler and raise it to actually stop the process
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

static void Terminal_handleSIGCONT(int sgn) {
    (void)sgn;
    
    // 1. Re-register our SIGTSTP handler
    signal(SIGTSTP, Terminal_handleSIGTSTP);
    
    // 2. Restore ncurses mode FIRST
    // This tells the terminal to enter "Visual/Application" mode.
    // If we don't do this first, the terminal might ignore our color commands
    // or reset them immediately after.
    refresh(); 
    
    // 3. Re-apply RGB Definitions
    // Now that we are in the correct mode, send the RGB hex codes.
    Terminal_resetColors();
    
    // 4. FORCE REPAINT
    // Mark the whole screen as 'dirty' and repaint it to ensure 
    // characters pick up the newly defined colors.
    redrawwin(stdscr);
    refresh();
}

static void Terminal_installSignalHandlers(void) {
   struct sigaction act;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_handler = Terminal_handleSIGTERM;
   
   sigaction(SIGINT, &act, &old_sig_handler[SIGINT]);
   sigaction(SIGTERM, &act, &old_sig_handler[SIGTERM]);
   sigaction(SIGQUIT, &act, &old_sig_handler[SIGQUIT]);

   // Register Resume Handler
   signal(SIGCONT, Terminal_handleSIGCONT);
   
   // Register Pause Handler
   signal(SIGTSTP, Terminal_handleSIGTSTP);
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
   } else {
      Terminal_colorScheme = COLORSCHEME_MONOCHROME;
   }
   
   Terminal_resetColors();
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
   // Restore old handlers
   sigaction(SIGINT, &old_sig_handler[SIGINT], NULL);
   sigaction(SIGTERM, &old_sig_handler[SIGTERM], NULL);
   sigaction(SIGQUIT, &old_sig_handler[SIGQUIT], NULL);
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

void Terminal_resetColors(void) {
   if (!has_colors()) { return; }

   short graph_color = COLOR_CYAN;
   short header_color = COLOR_GREEN;

   // Definition for fallback ANSI colors
   // These are used if custom RGB is NOT supported
   short p_colors[10] = {
      COLOR_MAGENTA, // 1. Deep Purple -> Magenta
      COLOR_CYAN,    // 2. Dark Teal -> Cyan
      COLOR_CYAN,    // 3. Dark Cyan -> Cyan
      COLOR_WHITE,   // 4. Pearl Aqua -> White
      COLOR_YELLOW,  // 5. Wheat -> Yellow
      COLOR_YELLOW,  // 6. Gold Orange -> Yellow
      COLOR_RED,     // 7. Burnt Caramel -> Red
      COLOR_RED,     // 8. Rusty Spice -> Red
      COLOR_RED,     // 9. Oxidized Iron -> Red
      COLOR_MAGENTA  // 10. Brown Red -> Magenta
   };

   bool can_set_rgb = can_change_color() && COLORS >= 256 && COLOR_PAIRS > PAIR_HEADER;
   
   if (can_set_rgb) {
      init_color(COLOR_HEX_HEADING, 953, 776, 184);    // #F3C62F
      init_color(COLOR_HEX_SIDE, 286, 827, 949);       // #49D3F2
      init_color(COLOR_HEX_CHART_UP, 718, 914, 757);   // #B7E9C1
      init_color(COLOR_HEX_CHART_DOWN, 682, 494, 949); // #AE7EF2

      // Sunset Palette (RGB 0-1000)
      init_color(C_INK_BLACK,      435, 176, 741); // #6f2dbd
      init_color(C_DARK_TEAL,      0,   372, 451); // #005f73
      init_color(C_DARK_CYAN,      39,  576, 588); // #0a9396
      init_color(C_PEARL_AQUA,     580, 823, 741); // #94d2bd
      init_color(C_WHEAT,          913, 847, 651); // #e9d8a6
      init_color(C_GOLD_ORANGE,    933, 607, 0);   // #ee9b00
      init_color(C_BURNT_CARAMEL,  792, 404, 8);   // #ca6702
      init_color(C_RUSTY_SPICE,    733, 243, 12);  // #bb3e03
      init_color(C_OXIDIZED_IRON,  682, 125, 70);  // #ae2012
      init_color(C_BROWN_RED,      608, 133, 149); // #9b2226

      graph_color = COLOR_HEX_CHART_UP;
      header_color = COLOR_HEX_HEADING;

      // Update palette array to use custom color IDs
      p_colors[0] = C_INK_BLACK;
      p_colors[1] = C_DARK_TEAL;
      p_colors[2] = C_DARK_CYAN;
      p_colors[3] = C_PEARL_AQUA;
      p_colors[4] = C_WHEAT;
      p_colors[5] = C_GOLD_ORANGE;
      p_colors[6] = C_BURNT_CARAMEL;
      p_colors[7] = C_RUSTY_SPICE;
      p_colors[8] = C_OXIDIZED_IRON;
      p_colors[9] = C_BROWN_RED;
   }

   init_pair(PAIR_GRAPH_LINE, graph_color, -1);
   init_pair(PAIR_HEADER, header_color, -1);
   init_pair(PAIR_HEADER_DIM, header_color, -1); 

   // Initialize Sunset Palette Pairs
   // Whether we used RGB or ANSI fallback, p_colors[] now holds the correct color ID
   for(int i = 0; i < 10; i++) {
       init_pair(PAIR_CHART_1 + i, p_colors[i], -1);
   }

   int graph_pair_attr = COLOR_PAIR(PAIR_GRAPH_LINE);
   int header_pair_attr = A_BOLD | COLOR_PAIR(PAIR_HEADER);
   int header_dim_attr = A_DIM | COLOR_PAIR(PAIR_HEADER_DIM);

   Terminal_colorSchemes[COLORSCHEME_DARK][GRAPH_LINE] = graph_pair_attr;
   Terminal_colorSchemes[COLORSCHEME_DARK][GRAPH_DOTS] = A_DIM | graph_pair_attr;
   Terminal_colorSchemes[COLORSCHEME_DARK][PANEL_HEADER] = header_pair_attr;
   Terminal_colorSchemes[COLORSCHEME_DARK][PANEL_HEADER_DIM] = header_dim_attr; 

   // Assign New Palette Attributes to Scheme
   for(int i = 0; i < 10; i++) {
       Terminal_colorSchemes[COLORSCHEME_DARK][CHART_COLOR_1 + i] = COLOR_PAIR(PAIR_CHART_1 + i);
   }
}

void Terminal_fatalError(const char* message) {
   Terminal_done();
   fprintf(stderr, "Fatal error: %s\n", message);
   exit(1);
}

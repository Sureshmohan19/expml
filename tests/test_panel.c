#include "Panel.h"
#include "Terminal.h"
#include <ncurses.h>

int main() {
    // Initialize terminal
    Terminal_init(true);
    
    // Create a test panel
    Panel* panel = Panel_new(5, 5, 40, 15, "Test Panel");
    panel->has_focus = true;
    
    // Add some test items
    for (int i = 0; i < 50; i++) {
        char text[100];
        snprintf(text, sizeof(text), "Item %d - This is a test item", i);
        Panel_addItem(panel, text, NULL);
    }
    
    // Main loop
    bool quit = false;
    while (!quit) {
        // Draw panel
        clear();
        attron(Terminal_colors[TEXT_NORMAL]);
        mvprintw(0, 0, "Panel Test - q to quit");
        mvprintw(1, 0, "Selected: %d/%d", 
                Panel_getSelectedIndex(panel), 
                Panel_getItemCount(panel));
        attroff(Terminal_colors[TEXT_NORMAL]);
        
        Panel_draw(panel, false);
        refresh();
        
        // Handle input
        int ch = Terminal_readKey();
        
        if (ch == 'q' || ch == 'Q') {
            quit = true;
        } else {
            Panel_onKey(panel, ch);
        }
    }
    
    // Cleanup
    Panel_delete(panel);
    Terminal_done();
    
    return 0;
}

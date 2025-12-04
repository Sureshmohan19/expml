#ifndef CONSTANTS_H
#define CONSTANTS_H

// --- Metric Cards (Charts) ---
// The fixed height of a single card in the grid
#define METRIC_CARD_HEIGHT 12       

// The absolute minimum width a card can shrink to before we force a new row.
// Reduced to 20 to allow 4 columns on 80-width, or 3 columns with sidebars.
#define METRIC_MIN_WIDTH 40    

// --- Layout Constraints ---
// The preferred width for Run/System sidebars
#define SIDEBAR_DEFAULT_WIDTH 35

// The absolute minimum width we allow sidebars to shrink to
#define SIDEBAR_MIN_WIDTH 20

// The minimum width we want to preserve for the center (Metrics) panel
// If available space is less than this, we start shrinking sidebars.
#define MAIN_PANEL_MIN_WIDTH 30

#endif // CONSTANTS_H
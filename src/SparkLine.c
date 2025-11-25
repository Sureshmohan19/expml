#include "SparkLine.h"
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Unicode Block Characters (Levels 1 to 8)
static const char* BLOCKS[] = {
    " ", "▂", "▃", "▄", "▅", "▆", "▇", "█"
};

void Sparkline_draw(const float* values, size_t count, int y, int x, int width, int height, int color) {
    if (!values || count == 0) return;

    // 1. Find Min/Max
    float min = values[0];
    float max = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }
    float range = max - min;
    if (range == 0) range = 1.0f;

    int steps = (count < (size_t)width) ? (int)count : width;

    attron(color);

    // 2. Draw Columns
    for (int col = 0; col < steps; col++) {
        int data_idx = (col * count) / steps;
        float val = values[data_idx];

        // Normalize to total number of sub-blocks (height * 8)
        // e.g., if height is 3, we have 24 levels of granularity
        float normalized = (val - min) / range;
        int total_levels = (int)(normalized * (height * 8.0f));

        // Draw from bottom up
        for (int row = 0; row < height; row++) {
            int screen_y = y + (height - 1) - row;
            int screen_x = x + col;

            // How many levels exist in this row?
            // Row 0 (bottom) handles levels 0-7
            // Row 1 (middle) handles levels 8-15
            int row_level = total_levels - (row * 8);

            if (row_level <= 0) {
                mvprintw(screen_y, screen_x, " ");
            } else if (row_level >= 8) {
                mvprintw(screen_y, screen_x, "█");
            } else {
                mvprintw(screen_y, screen_x, "%s", BLOCKS[row_level]);
            }
        }
    }
    attroff(color);
}

void Sparkline_generate(const float* values, size_t count, char* buffer, int width) {
    if (!values || count == 0 || width <= 0) {
        buffer[0] = '\0';
        return;
    }

    // 1. Find Min/Max to normalize the data
    float min = values[0];
    float max = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }

    float range = max - min;
    if (range == 0) range = 1.0f; // Avoid division by zero

    // 2. Determine step size
    // If we have more data than width, we skip some points (decimation)
    // If we have less data, we just use what we have
    int steps = (count < (size_t)width) ? (int)count : width;
    
    buffer[0] = '\0'; // Initialize empty string

    // 3. Generate String
    for (int i = 0; i < steps; i++) {
        // Map screen position 'i' to data index
        // If decimation is needed, this picks representative points
        int data_idx = (i * count) / steps;
        
        float val = values[data_idx];
        
        // Normalize to 0..7 (8 levels)
        int level = (int)((val - min) / range * 7.0f);
        if (level < 0) level = 0;
        if (level > 7) level = 7;
        
        // Append UTF-8 character
        strcat(buffer, BLOCKS[level]);
    }
}

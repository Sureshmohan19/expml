#include "SparkLine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

// Braille Map (2x4 grid)
static const int BRAILLE_MAP[4][2] = {
    {0x01, 0x08},
    {0x02, 0x10},
    {0x04, 0x20},
    {0x40, 0x80}
};

// Standard Bresenham Line Algorithm
static void draw_virtual_line(unsigned char* grid, int w, int h, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) {
            grid[y0 * w + x0] = 1;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void Sparkline_draw(const float* values, size_t count, int y, int x, int width, int height, int color) {
    if (!values || count == 0) return;

    // 1. Calculate Min/Max for scaling
    float min = values[0];
    float max = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }
    float range = max - min;
    if (range == 0) range = 1.0f;

    // 2. Setup Virtual Grid (2x width, 4x height)
    int v_width = width * 2;
    int v_height = height * 4;
    
    // Use calloc so the grid is zeroed out immediately
    unsigned char* grid = calloc(v_width * v_height, sizeof(unsigned char));
    if (!grid) return;

    // 3. PURE SPATIAL BINNING (The "Go" Logic)
    // No "smart sampling" loops. No buckets. 
    // Just project every point and connect them.
    
    int prev_vx = -1;
    int prev_vy = -1;

    for (size_t i = 0; i < count; i++) {
        // Project X: map index to virtual width
        int vx = (int)((double)i * (v_width - 1) / (count - 1));
        
        // Project Y: map value to virtual height
        // Note: 0 is bottom in grid logic, so we map directly.
        float norm = (values[i] - min) / range;
        int vy = (int)(norm * (v_height - 1));

        // Clamp coordinates (safety)
        if (vx < 0) vx = 0;
        if (vx >= v_width) vx = v_width - 1;
        if (vy < 0) vy = 0;
        if (vy >= v_height) vy = v_height - 1;

        // Draw
        if (i == 0) {
            // First point, just set the pixel
            grid[vy * v_width + vx] = 1;
        } else {
            // Optimization: Only draw if the point moved in the grid.
            // This saves CPU if you have 10,000 points mapping to the same x,y.
            if (vx != prev_vx || vy != prev_vy) {
                draw_virtual_line(grid, v_width, v_height, prev_vx, prev_vy, vx, vy);
            }
        }
        
        prev_vx = vx;
        prev_vy = vy;
    }

    // 4. Render Grid to Braille Characters
    attron(color);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int braille_char = 0x2800;

            // Check the 2x4 block
            for (int sub_x = 0; sub_x < 2; sub_x++) {
                for (int sub_y = 0; sub_y < 4; sub_y++) {
                    int vx = (col * 2) + sub_x;
                    
                    // Coordinate mapping:
                    // Visual Row 0 is the TOP of the screen.
                    // But in our grid, high Y = high value (TOP).
                    // So we need to map Visual Top to Grid Top.
                    
                    int grid_block_bottom = (height - 1 - row) * 4; 
                    int vy = grid_block_bottom + (3 - sub_y);

                    if (vx < v_width && vy < v_height && vy >= 0) {
                        if (grid[vy * v_width + vx]) {
                            braille_char |= BRAILLE_MAP[sub_y][sub_x];
                        }
                    }
                }
            }

            if (braille_char == 0x2800) {
                mvaddch(y + row, x + col, ' ');
            } else {
                char utf8[4];
                // Standard Braille Unicode offset
                utf8[0] = 0xE2;
                utf8[1] = 0xA0 | ((braille_char >> 6) & 0x03);
                utf8[2] = 0x80 | (braille_char & 0x3F);
                utf8[3] = '\0';
                mvprintw(y + row, x + col, "%s", utf8);
            }
        }
    }

    attroff(color);
    free(grid);
}

void Sparkline_generate(const float* values, size_t count, char* buffer, int width) {
    if (buffer) buffer[0] = '\0';
}
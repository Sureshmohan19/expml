#ifndef EXPML_SPARKLINE_H
#define EXPML_SPARKLINE_H

#include <stddef.h>

// Add this function:
/*
 * Draw a multi-line chart directly to ncurses
 * values: data array
 * count: data count
 * y, x: Top-left position
 * width: Width in columns
 * height: Height in rows
 * color: Color pair to use
 */
void Sparkline_draw(const float* values, size_t count, int y, int x, int width, int height, int color);

/*
 * Generates a block-character sparkline string.
 * 
 * values: Array of floats
 * count: Number of values
 * buffer: Output buffer to write string to
 * width: Max width in characters (buffer must be width*3 + 1 for UTF-8 safety)
 */
void Sparkline_generate(const float* values, size_t count, char* buffer, int width);

#endif

#ifndef EXPML_SPARKLINE_H
#define EXPML_SPARKLINE_H

#include <stddef.h>

void Sparkline_draw(const float* values, size_t count, int y, int x, int width, int height, int color);

#endif
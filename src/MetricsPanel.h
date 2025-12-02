#ifndef EXPML_METRICSPANEL_H
#define EXPML_METRICSPANEL_H

#include "Panel.h"

// Creates a new Metrics Grid Panel
Panel* MetricsPanel_new(int x, int y, int w, int h);

// Adds a metric card to the grid
// If the panel was recently cleared, this resets the internal state automatically.
void MetricsPanel_addMetric(Panel* panel, const char* name, float current_val, const float* values, int count);

// Updates layout when terminal resizes
void MetricsPanel_updateSize(Panel* panel, int w, int h);

#endif
#ifndef EXPML_METRICSPANEL_H
#define EXPML_METRICSPANEL_H

#include "Panel.h"

/*
 * Create the Metrics Panel (Middle Column)
 */
Panel* MetricsPanel_new(int x, int y, int w, int h);

/*
 * Add a metric trace to the panel
 * name: "loss", "accuracy", etc.
 * values: Array of historical values (will be copied)
 * count: Number of values
 */
void MetricsPanel_addMetric(Panel* this, const char* name, float current_val, const float* values, int count);

#endif

#ifndef EXPML_METRICSPANEL_H
#define EXPML_METRICSPANEL_H

#include "Panel.h"

Panel* MetricsPanel_new(int x, int y, int w, int h);
void MetricsPanel_addMetric(Panel* this, const char* name, float current_val, const float* values, int count);
void MetricsPanel_updateSize(Panel* p, int w, int h); // Call this when terminal resizes

#endif

#ifndef EXPML_DATALOADER_H
#define EXPML_DATALOADER_H

#include "Panel.h"

/*
 * DataLoader - Bridges Storage and UI Panels
 * Reads raw data from Storage and populates the specific Panel widgets.
 */

/*
 * Reads metrics.jsonl from the run path and populates:
 * 1. metricsPanel: With sparkline charts for normal metrics
 * 2. systemPanel: With key-value pairs for "system/" metrics
 */
void DataLoader_loadMetrics(const char* run_path, Panel* metricsPanel, Panel* systemPanel);

#endif

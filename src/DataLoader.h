#ifndef EXPML_DATALOADER_H
#define EXPML_DATALOADER_H

#include "Panel.h"

// Loads metrics and system data from a run directory and populates the corresponding panels
void DataLoader_loadMetrics(const char* run_path, Panel* metricsPanel, Panel* systemPanel);

#endif

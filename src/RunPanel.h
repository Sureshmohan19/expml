#ifndef EXPML_RUNPANEL_H
#define EXPML_RUNPANEL_H

#include "Panel.h"
#include "Storage.h"

/*
 * Create the Run Overview Panel (Left Column)
 * Populates it with the placeholder data from the screenshot.
 */
Panel* RunPanel_new(int x, int y, int w, int h);

/*
 * Update the data displayed in the panel
 * Can pass NULL for any argument to skip updating that section
 */
void RunPanel_setData(Panel* this, RunConfig* config, RunMetadata* meta, RunSummary* summary);

#endif

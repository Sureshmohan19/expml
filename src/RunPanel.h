#ifndef EXPML_RUNPANEL_H
#define EXPML_RUNPANEL_H

#include "Panel.h"
#include "Storage.h"

Panel* RunPanel_new(int x, int y, int w, int h);
void RunPanel_setData(Panel* this, RunConfig* config, RunMetadata* meta, RunSummary* summary);

#endif

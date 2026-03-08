#pragma once

#include "dockmodel.h"

typedef struct DockModelValidateStats
{
	int nRepairs;
	int nErrors;
} DockModelValidateStats;

BOOL DockModelValidateAndRepairMainLayout(DockModelNode** ppRootNode, DockModelValidateStats* pStats);

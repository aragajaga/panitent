#pragma once

#include "precomp.h"

#include "dockviewcatalog.h"

typedef struct DockFloatingLayoutEntry
{
	PanitentDockViewId nViewId;
	RECT rcWindow;
	int iDockSizeHint;
} DockFloatingLayoutEntry;

typedef struct DockFloatingLayoutFileModel
{
	int nEntries;
	DockFloatingLayoutEntry entries[32];
} DockFloatingLayoutFileModel;

BOOL DockFloatingLayout_SaveToFile(const DockFloatingLayoutFileModel* pModel, PCWSTR pszFilePath);
BOOL DockFloatingLayout_LoadFromFile(PCWSTR pszFilePath, DockFloatingLayoutFileModel* pModel);

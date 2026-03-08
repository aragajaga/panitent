#pragma once

#include "precomp.h"

#include "dockmodel.h"
#include "persistload.h"
#include "dockviewcatalog.h"
#include "floatingdockpolicy.h"

typedef struct DockFloatingLayoutEntry
{
	RECT rcWindow;
	int iDockSizeHint;
	FloatingDockChildHostKind nChildKind;
	PanitentDockViewId nViewId;
	DockModelNode* pLayoutModel;
} DockFloatingLayoutEntry;

typedef struct DockFloatingLayoutFileModel
{
	int nEntries;
	DockFloatingLayoutEntry entries[32];
} DockFloatingLayoutFileModel;

BOOL DockFloatingLayout_SaveToFile(const DockFloatingLayoutFileModel* pModel, PCWSTR pszFilePath);
BOOL DockFloatingLayout_LoadFromFileEx(PCWSTR pszFilePath, DockFloatingLayoutFileModel* pModel, PersistLoadStatus* pStatus);
BOOL DockFloatingLayout_LoadFromFile(PCWSTR pszFilePath, DockFloatingLayoutFileModel* pModel);
void DockFloatingLayout_Destroy(DockFloatingLayoutFileModel* pModel);

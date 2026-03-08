#pragma once

#include "docktypes.h"

typedef enum PanitentDockViewId
{
	PNT_DOCK_VIEW_NONE = 0,
	PNT_DOCK_VIEW_WORKSPACE,
	PNT_DOCK_VIEW_TOOLBOX,
	PNT_DOCK_VIEW_GLWINDOW,
	PNT_DOCK_VIEW_PALETTE,
	PNT_DOCK_VIEW_LAYERS,
	PNT_DOCK_VIEW_OPTIONBAR
} PanitentDockViewId;

PanitentDockViewId PanitentDockViewCatalog_Find(DockNodeRole nRole, PCWSTR pszName);
BOOL PanitentDockViewCatalog_IsKnown(DockNodeRole nRole, PCWSTR pszName);
PanitentDockViewId PanitentDockViewCatalog_FindForWindow(PCWSTR pszClassName, PCWSTR pszTitle);

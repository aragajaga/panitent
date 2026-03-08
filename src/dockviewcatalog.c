#include "precomp.h"

#include "dockviewcatalog.h"

PanitentDockViewId PanitentDockViewCatalog_Find(DockNodeRole nRole, PCWSTR pszName)
{
	if (!pszName || !pszName[0])
	{
		return PNT_DOCK_VIEW_NONE;
	}

	if (nRole == DOCK_ROLE_WORKSPACE && wcscmp(pszName, L"WorkspaceContainer") == 0)
	{
		return PNT_DOCK_VIEW_WORKSPACE;
	}

	if (nRole != DOCK_ROLE_PANEL)
	{
		return PNT_DOCK_VIEW_NONE;
	}

	if (wcscmp(pszName, L"Toolbox") == 0)
	{
		return PNT_DOCK_VIEW_TOOLBOX;
	}
	if (wcscmp(pszName, L"GLWindow") == 0)
	{
		return PNT_DOCK_VIEW_GLWINDOW;
	}
	if (wcscmp(pszName, L"Palette") == 0)
	{
		return PNT_DOCK_VIEW_PALETTE;
	}
	if (wcscmp(pszName, L"Layers") == 0)
	{
		return PNT_DOCK_VIEW_LAYERS;
	}
	if (wcscmp(pszName, L"Option Bar") == 0)
	{
		return PNT_DOCK_VIEW_OPTIONBAR;
	}

	return PNT_DOCK_VIEW_NONE;
}

BOOL PanitentDockViewCatalog_IsKnown(DockNodeRole nRole, PCWSTR pszName)
{
	return PanitentDockViewCatalog_Find(nRole, pszName) != PNT_DOCK_VIEW_NONE;
}

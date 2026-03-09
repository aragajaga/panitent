#include "precomp.h"

#include "dockviewcatalog.h"

PCWSTR PanitentDockViewCatalog_GetCanonicalName(PanitentDockViewId nViewId)
{
	switch (nViewId)
	{
	case PNT_DOCK_VIEW_WORKSPACE:
		return L"WorkspaceContainer";
	case PNT_DOCK_VIEW_TOOLBOX:
		return L"Toolbox";
	case PNT_DOCK_VIEW_GLWINDOW:
		return L"GLWindow";
	case PNT_DOCK_VIEW_PALETTE:
		return L"Palette";
	case PNT_DOCK_VIEW_LAYERS:
		return L"Layers";
	case PNT_DOCK_VIEW_OPTIONBAR:
		return L"Option Bar";
	default:
		return NULL;
	}
}

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
	if (wcscmp(pszName, L"LayersWindow") == 0)
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

PanitentDockViewId PanitentDockViewCatalog_FindForWindow(PCWSTR pszClassName, PCWSTR pszTitle)
{
	if (pszTitle && pszTitle[0] != L'\0')
	{
		PanitentDockViewId nByTitle = PanitentDockViewCatalog_Find(DOCK_ROLE_PANEL, pszTitle);
		if (nByTitle != PNT_DOCK_VIEW_NONE)
		{
			return nByTitle;
		}
	}

	if (!pszClassName || !pszClassName[0])
	{
		return PNT_DOCK_VIEW_NONE;
	}

	if (wcscmp(pszClassName, L"__ToolboxWindow") == 0)
	{
		return PNT_DOCK_VIEW_TOOLBOX;
	}
	if (wcscmp(pszClassName, L"GLWindowClass") == 0)
	{
		return PNT_DOCK_VIEW_GLWINDOW;
	}
	if (wcscmp(pszClassName, L"PaletteWindowClass") == 0)
	{
		return PNT_DOCK_VIEW_PALETTE;
	}
	if (wcscmp(pszClassName, L"__LayersWindow") == 0)
	{
		return PNT_DOCK_VIEW_LAYERS;
	}
	if (wcscmp(pszClassName, L"__OptionBarWindow") == 0)
	{
		return PNT_DOCK_VIEW_OPTIONBAR;
	}

	return PNT_DOCK_VIEW_NONE;
}

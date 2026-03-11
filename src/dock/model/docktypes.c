#include "precomp.h"

#include "docktypes.h"

static BOOL DockNodeRole_NameStartsWith(PCWSTR pszName, PCWSTR pszPrefix)
{
	if (!pszName || !pszPrefix)
	{
		return FALSE;
	}

	size_t cchPrefix = wcslen(pszPrefix);
	if (cchPrefix == 0)
	{
		return FALSE;
	}

	return wcsncmp(pszName, pszPrefix, cchPrefix) == 0;
}

DockNodeRole DockNodeRole_Resolve(DockNodeRole role, PCWSTR pszName)
{
	if (role != DOCK_ROLE_UNKNOWN)
	{
		return role;
	}

	if (!pszName || !pszName[0])
	{
		return DOCK_ROLE_UNKNOWN;
	}

	if (wcscmp(pszName, L"Root") == 0)
	{
		return DOCK_ROLE_ROOT;
	}

	if (wcscmp(pszName, L"WorkspaceContainer") == 0)
	{
		return DOCK_ROLE_WORKSPACE;
	}

	if (wcscmp(pszName, L"DockShell.ZoneStack") == 0)
	{
		return DOCK_ROLE_ZONE_STACK_SPLIT;
	}

	if (wcscmp(pszName, L"DockShell.PanelSplit") == 0)
	{
		return DOCK_ROLE_PANEL_SPLIT;
	}

	if (DockNodeRole_NameStartsWith(pszName, L"DockZone."))
	{
		return DOCK_ROLE_ZONE;
	}

	if (DockNodeRole_NameStartsWith(pszName, L"DockShell."))
	{
		return DOCK_ROLE_SHELL_SPLIT;
	}

	return DOCK_ROLE_PANEL;
}

BOOL DockNodeRole_IsRoot(DockNodeRole role, PCWSTR pszName)
{
	return DockNodeRole_Resolve(role, pszName) == DOCK_ROLE_ROOT;
}

BOOL DockNodeRole_IsZone(DockNodeRole role, PCWSTR pszName)
{
	return DockNodeRole_Resolve(role, pszName) == DOCK_ROLE_ZONE;
}

BOOL DockNodeRole_IsStructural(DockNodeRole role, PCWSTR pszName)
{
	switch (DockNodeRole_Resolve(role, pszName))
	{
	case DOCK_ROLE_ROOT:
	case DOCK_ROLE_ZONE:
	case DOCK_ROLE_SHELL_SPLIT:
	case DOCK_ROLE_ZONE_STACK_SPLIT:
	case DOCK_ROLE_PANEL_SPLIT:
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL DockNodeRole_UsesProportionalGrip(DockNodeRole role, PCWSTR pszName)
{
	switch (DockNodeRole_Resolve(role, pszName))
	{
	case DOCK_ROLE_ZONE_STACK_SPLIT:
	case DOCK_ROLE_PANEL_SPLIT:
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL DockNodeRole_IsCorePanel(DockNodeRole role, PCWSTR pszName)
{
	switch (DockNodeRole_Resolve(role, pszName))
	{
	case DOCK_ROLE_ROOT:
	case DOCK_ROLE_ZONE:
	case DOCK_ROLE_SHELL_SPLIT:
	case DOCK_ROLE_ZONE_STACK_SPLIT:
	case DOCK_ROLE_PANEL_SPLIT:
	case DOCK_ROLE_WORKSPACE:
		return TRUE;
	default:
		return FALSE;
	}
}

#include "precomp.h"

#include "dockpolicy.h"

static BOOL DockPolicy_NameStartsWith(PCWSTR pszName, PCWSTR pszPrefix)
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

static BOOL DockPolicy_IsCorePanelName(PCWSTR pszName)
{
	if (!pszName || !pszName[0])
	{
		return FALSE;
	}

	if (wcscmp(pszName, L"WorkspaceContainer") == 0)
	{
		return TRUE;
	}

	if (wcscmp(pszName, L"Root") == 0)
	{
		return TRUE;
	}

	if (DockPolicy_NameStartsWith(pszName, L"DockZone.") || DockPolicy_NameStartsWith(pszName, L"DockShell."))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL DockPolicy_CanUndockPanelName(PCWSTR pszName)
{
	return DockPolicy_IsCorePanelName(pszName) ? FALSE : TRUE;
}

BOOL DockPolicy_CanClosePanelName(PCWSTR pszName)
{
	return DockPolicy_IsCorePanelName(pszName) ? FALSE : TRUE;
}

BOOL DockPolicy_CanPinPanelName(PCWSTR pszName)
{
	return DockPolicy_IsCorePanelName(pszName) ? FALSE : TRUE;
}

void DockPolicy_ResolveZoneTabClick(BOOL bHasClickedTab, BOOL bClickedIsActive, BOOL bWasCollapsed, DockPolicyZoneTabClickResult* pResult)
{
	if (!pResult)
	{
		return;
	}

	pResult->bCollapsed = bWasCollapsed;
	pResult->bActivateClickedTab = FALSE;

	if (!bHasClickedTab)
	{
		pResult->bCollapsed = bWasCollapsed ? FALSE : TRUE;
		return;
	}

	if (bClickedIsActive)
	{
		pResult->bCollapsed = bWasCollapsed ? FALSE : TRUE;
		return;
	}

	pResult->bCollapsed = FALSE;
	pResult->bActivateClickedTab = TRUE;
}

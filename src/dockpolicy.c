#include "precomp.h"

#include "dockpolicy.h"

BOOL DockPolicy_CanUndockPanelName(PCWSTR pszName)
{
	return DockPolicy_CanUndockPanel(DOCK_ROLE_UNKNOWN, pszName);
}

BOOL DockPolicy_CanClosePanelName(PCWSTR pszName)
{
	return DockPolicy_CanClosePanel(DOCK_ROLE_UNKNOWN, pszName);
}

BOOL DockPolicy_CanPinPanelName(PCWSTR pszName)
{
	return DockPolicy_CanPinPanel(DOCK_ROLE_UNKNOWN, pszName);
}

BOOL DockPolicy_CanUndockPanel(DockNodeRole role, PCWSTR pszName)
{
	return DockNodeRole_IsCorePanel(role, pszName) ? FALSE : TRUE;
}

BOOL DockPolicy_CanClosePanel(DockNodeRole role, PCWSTR pszName)
{
	return DockNodeRole_IsCorePanel(role, pszName) ? FALSE : TRUE;
}

BOOL DockPolicy_CanPinPanel(DockNodeRole role, PCWSTR pszName)
{
	return DockNodeRole_IsCorePanel(role, pszName) ? FALSE : TRUE;
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

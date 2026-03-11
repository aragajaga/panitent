#pragma once

#include "precomp.h"
#include "docktypes.h"

typedef struct DockPolicyZoneTabClickResult DockPolicyZoneTabClickResult;
struct DockPolicyZoneTabClickResult
{
	BOOL bCollapsed;
	BOOL bActivateClickedTab;
};

BOOL DockPolicy_CanUndockPanelName(PCWSTR pszName);
BOOL DockPolicy_CanClosePanelName(PCWSTR pszName);
BOOL DockPolicy_CanPinPanelName(PCWSTR pszName);
BOOL DockPolicy_CanUndockPanel(DockNodeRole role, PCWSTR pszName);
BOOL DockPolicy_CanClosePanel(DockNodeRole role, PCWSTR pszName);
BOOL DockPolicy_CanPinPanel(DockNodeRole role, PCWSTR pszName);
void DockPolicy_ResolveZoneTabClick(BOOL bHasClickedTab, BOOL bClickedIsActive, BOOL bWasCollapsed, DockPolicyZoneTabClickResult* pResult);

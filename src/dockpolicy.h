#pragma once

#include "precomp.h"

typedef struct DockPolicyZoneTabClickResult DockPolicyZoneTabClickResult;
struct DockPolicyZoneTabClickResult
{
	BOOL bCollapsed;
	BOOL bActivateClickedTab;
};

BOOL DockPolicy_CanUndockPanelName(PCWSTR pszName);
BOOL DockPolicy_CanClosePanelName(PCWSTR pszName);
void DockPolicy_ResolveZoneTabClick(BOOL bHasClickedTab, BOOL bClickedIsActive, BOOL bWasCollapsed, DockPolicyZoneTabClickResult* pResult);

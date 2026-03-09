#pragma once

#include "dockhost.h"

#define DOCK_ZONE_MAX_TABS 32

int DockZone_GetPanels(TreeNode* pZoneNode, TreeNode** ppNodes, int cCapacity);
int DockZone_GetPanelsByCollapsed(TreeNode* pZoneNode, TreeNode** ppNodes, int cCapacity, BOOL bCollapsed);
void DockZone_EnsureActiveTab(TreeNode* pZoneNode);

#pragma once

#include "dockhost.h"

#define DOCK_ZONE_MAX_TABS 32

int DockZone_GetPanels(TreeNode* pZoneNode, TreeNode** ppNodes, int cCapacity);
int DockZone_GetPanelsByCollapsed(TreeNode* pZoneNode, TreeNode** ppNodes, int cCapacity, BOOL bCollapsed);
void DockZone_EnsureActiveTab(TreeNode* pZoneNode);
void DockHostZone_UpdateTabGutters(DockHostWindow* pDockHostWindow, int iZoneTabGutter, int* pLeft, int* pRight, int* pTop, int* pBottom);
void DockHostZone_Sync(DockHostWindow* pDockHostWindow, int iZoneTabGutter, int* pLeft, int* pRight, int* pTop, int* pBottom);
void DockHostZone_SyncHostGutters(DockHostWindow* pDockHostWindow, int iZoneTabGutter);

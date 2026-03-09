#include "precomp.h"

#include "dockhostzone.h"
#include "dockhostmetrics.h"
#include "dockhosttree.h"

static void DockZone_CollectPanels(TreeNode* pNode, TreeNode** ppNodes, int cCapacity, int* pnCount)
{
	if (!pNode || !ppNodes || !pnCount || *pnCount >= cCapacity)
	{
		return;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData && pDockData->hWnd && IsWindow(pDockData->hWnd))
	{
		ppNodes[*pnCount] = pNode;
		(*pnCount)++;
		return;
	}

	DockZone_CollectPanels(pNode->node1, ppNodes, cCapacity, pnCount);
	DockZone_CollectPanels(pNode->node2, ppNodes, cCapacity, pnCount);
}

int DockZone_GetPanels(TreeNode* pZoneNode, TreeNode** ppNodes, int cCapacity)
{
	if (!pZoneNode || !ppNodes || cCapacity <= 0)
	{
		return 0;
	}

	int nCount = 0;
	DockZone_CollectPanels(pZoneNode->node1, ppNodes, cCapacity, &nCount);
	return nCount;
}

int DockZone_GetPanelsByCollapsed(TreeNode* pZoneNode, TreeNode** ppNodes, int cCapacity, BOOL bCollapsed)
{
	if (!pZoneNode || !ppNodes || cCapacity <= 0)
	{
		return 0;
	}

	TreeNode* panels[DOCK_ZONE_MAX_TABS] = { 0 };
	int nPanels = DockZone_GetPanels(pZoneNode, panels, ARRAYSIZE(panels));
	int nFiltered = 0;
	for (int i = 0; i < nPanels && nFiltered < cCapacity; ++i)
	{
		DockData* pDockData = panels[i] ? (DockData*)panels[i]->data : NULL;
		if (!pDockData || !pDockData->hWnd || !IsWindow(pDockData->hWnd))
		{
			continue;
		}

		if ((pDockData->bCollapsed ? TRUE : FALSE) != (bCollapsed ? TRUE : FALSE))
		{
			continue;
		}

		ppNodes[nFiltered++] = panels[i];
	}

	return nFiltered;
}

void DockZone_EnsureActiveTab(TreeNode* pZoneNode)
{
	if (!pZoneNode || !pZoneNode->data)
	{
		return;
	}

	DockData* pZoneData = (DockData*)pZoneNode->data;
	TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
	int nTabs = DockZone_GetPanelsByCollapsed(pZoneNode, tabs, ARRAYSIZE(tabs), FALSE);
	if (nTabs <= 0)
	{
		pZoneData->hWndActiveTab = NULL;
		return;
	}

	if (!pZoneData->hWndActiveTab || !IsWindow(pZoneData->hWndActiveTab))
	{
		DockData* pDockData = (DockData*)tabs[0]->data;
		pZoneData->hWndActiveTab = pDockData ? pDockData->hWnd : NULL;
		return;
	}

	for (int i = 0; i < nTabs; ++i)
	{
		DockData* pDockData = (DockData*)tabs[i]->data;
		if (pDockData && pDockData->hWnd == pZoneData->hWndActiveTab)
		{
			return;
		}
	}

	DockData* pDockData = (DockData*)tabs[0]->data;
	pZoneData->hWndActiveTab = pDockData ? pDockData->hWnd : NULL;
}

static int DockHostZone_GetSideTabGutter(DockHostWindow* pDockHostWindow, int nDockSide, int iZoneTabGutter)
{
    TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, nDockSide);
    if (!pZoneNode)
    {
        return 0;
    }

    TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
    int nTabs = DockZone_GetPanelsByCollapsed(pZoneNode, tabs, ARRAYSIZE(tabs), TRUE);
    return (nTabs > 0) ? iZoneTabGutter : 0;
}

void DockHostZone_UpdateTabGutters(DockHostWindow* pDockHostWindow, int iZoneTabGutter, int* pLeft, int* pRight, int* pTop, int* pBottom)
{
    if (!pDockHostWindow)
    {
        if (pLeft) *pLeft = 0;
        if (pRight) *pRight = 0;
        if (pTop) *pTop = 0;
        if (pBottom) *pBottom = 0;
        return;
    }

    if (pLeft) *pLeft = DockHostZone_GetSideTabGutter(pDockHostWindow, DKS_LEFT, iZoneTabGutter);
    if (pRight) *pRight = DockHostZone_GetSideTabGutter(pDockHostWindow, DKS_RIGHT, iZoneTabGutter);
    if (pTop) *pTop = DockHostZone_GetSideTabGutter(pDockHostWindow, DKS_TOP, iZoneTabGutter);
    if (pBottom) *pBottom = DockHostZone_GetSideTabGutter(pDockHostWindow, DKS_BOTTOM, iZoneTabGutter);
}

void DockHostZone_Sync(DockHostWindow* pDockHostWindow, int iZoneTabGutter, int* pLeft, int* pRight, int* pTop, int* pBottom)
{
    if (!pDockHostWindow)
    {
        DockHostZone_UpdateTabGutters(NULL, iZoneTabGutter, pLeft, pRight, pTop, pBottom);
        return;
    }

    const int sides[] = { DKS_LEFT, DKS_RIGHT, DKS_TOP, DKS_BOTTOM };
    for (int i = 0; i < ARRAYSIZE(sides); ++i)
    {
        TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, sides[i]);
        DockZone_EnsureActiveTab(pZoneNode);
    }

    DockHostZone_UpdateTabGutters(pDockHostWindow, iZoneTabGutter, pLeft, pRight, pTop, pBottom);
}

void DockHostZone_SyncHostGutters(DockHostWindow* pDockHostWindow, int iZoneTabGutter)
{
    int iLeft = 0;
    int iRight = 0;
    int iTop = 0;
    int iBottom = 0;
    DockHostZone_Sync(pDockHostWindow, iZoneTabGutter, &iLeft, &iRight, &iTop, &iBottom);
    DockHostMetrics_SetRootGutters(iLeft, iRight, iTop, iBottom);
}

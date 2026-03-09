#include "precomp.h"

#include "dockhostzone.h"

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

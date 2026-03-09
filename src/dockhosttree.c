#include "precomp.h"

#include "dockhosttree.h"

#include "dockgroup.h"

static TreeNode* DockNode_FindByName(TreeNode* pNode, PCWSTR pszName)
{
	if (!pNode || !pszName)
	{
		return NULL;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData && wcscmp(pDockData->lpszName, pszName) == 0)
	{
		return pNode;
	}

	TreeNode* pFound = DockNode_FindByName(pNode->node1, pszName);
	if (pFound)
	{
		return pFound;
	}

	return DockNode_FindByName(pNode->node2, pszName);
}

static PCWSTR DockHostTree_GetZoneName(int nDockSide)
{
	switch (nDockSide)
	{
	case DKS_LEFT:
		return L"DockZone.Left";
	case DKS_RIGHT:
		return L"DockZone.Right";
	case DKS_TOP:
		return L"DockZone.Top";
	case DKS_BOTTOM:
		return L"DockZone.Bottom";
	default:
		return NULL;
	}
}

TreeNode* DockNode_FindParent(TreeNode* root, TreeNode* node)
{
	if (!root || !node)
	{
		return NULL;
	}

	TreeNode* pFoundNode = NULL;
	if (root->node1)
	{
		if (root->node1 == node)
		{
			return root;
		}
		pFoundNode = DockNode_FindParent(root->node1, node);
	}

	if (!pFoundNode && root->node2)
	{
		if (root->node2 == node)
		{
			return root;
		}
		pFoundNode = DockNode_FindParent(root->node2, node);
	}

	return pFoundNode;
}

TreeNode* DockNode_FindByHWND(TreeNode* pNode, HWND hWnd)
{
	if (!pNode || !hWnd)
	{
		return NULL;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData && pDockData->hWnd == hWnd)
	{
		return pNode;
	}

	TreeNode* pFound = DockNode_FindByHWND(pNode->node1, hWnd);
	if (pFound)
	{
		return pFound;
	}

	return DockNode_FindByHWND(pNode->node2, hWnd);
}

TreeNode* DockNode_FindZoneBySide(TreeNode* pNode, int nDockSide)
{
	if (!pNode)
	{
		return NULL;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData &&
		DockNodeRole_IsZone(pDockData->nRole, pDockData->lpszName) &&
		pDockData->nDockSide == nDockSide)
	{
		return pNode;
	}

	TreeNode* pFound = DockNode_FindZoneBySide(pNode->node1, nDockSide);
	if (pFound)
	{
		return pFound;
	}

	return DockNode_FindZoneBySide(pNode->node2, nDockSide);
}

TreeNode* DockHostWindow_GetZoneNode(DockHostWindow* pDockHostWindow, int nDockSide)
{
	if (!pDockHostWindow || nDockSide == DKS_NONE)
	{
		return NULL;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	TreeNode* pZoneNode = DockNode_FindZoneBySide(pRoot, nDockSide);
	if (pZoneNode)
	{
		return pZoneNode;
	}

	PCWSTR pszZoneName = DockHostTree_GetZoneName(nDockSide);
	if (!pszZoneName)
	{
		return NULL;
	}

	return DockNode_FindByName(pRoot, pszZoneName);
}

TreeNode* DockHostWindow_FindOwningZoneNode(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode)
{
	TreeNode* pGroupNode;
	if (!pDockHostWindow || !pPanelNode)
	{
		return NULL;
	}

	pGroupNode = DockGroup_FindOwningGroup(DockHostWindow_GetRoot(pDockHostWindow), pPanelNode);
	if (!pGroupNode || DockGroup_GetNodeKind(pGroupNode) != DOCK_GROUP_TOOL_PANE)
	{
		return NULL;
	}

	return pGroupNode;
}

int DockHostWindow_GetPanelDockSide(DockHostWindow* pDockHostWindow, HWND hWndPanel)
{
	if (!pDockHostWindow || !hWndPanel || !IsWindow(hWndPanel))
	{
		return DKS_NONE;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	TreeNode* pPanelNode = DockNode_FindByHWND(pRoot, hWndPanel);
	if (!pPanelNode)
	{
		return DKS_NONE;
	}

	TreeNode* pZoneNode = DockHostWindow_FindOwningZoneNode(pDockHostWindow, pPanelNode);
	if (!pZoneNode || !pZoneNode->data)
	{
		return DKS_NONE;
	}

	return ((DockData*)pZoneNode->data)->nDockSide;
}

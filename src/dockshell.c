#include "precomp.h"

#include "dockshell.h"
#include "docklayout.h"

static void DockShell_InitDockData(
	DockData* pDockData,
	DockNodeRole nRole,
	PCWSTR pszName,
	int nDockSide,
	int iGripPos,
	DWORD dwStyle,
	BOOL bShowCaption)
{
	if (!pDockData)
	{
		return;
	}

	memset(pDockData, 0, sizeof(DockData));
	pDockData->dwStyle = dwStyle;
	pDockData->fGripPos = -1.0f;
	pDockData->iGripPos = (short)iGripPos;
	pDockData->bShowCaption = bShowCaption;
	pDockData->bCollapsed = FALSE;
	pDockData->hWndActiveTab = NULL;
	pDockData->nRole = nRole;
	pDockData->nDockSide = nDockSide;
	if (pszName)
	{
		wcscpy_s(pDockData->lpszName, MAX_PATH, pszName);
	}
}

static TreeNode* DockShell_CreateNode(
	DockNodeRole nRole,
	PCWSTR pszName,
	int nDockSide,
	int iGripPos,
	DWORD dwStyle,
	BOOL bShowCaption)
{
	TreeNode* pNode = (TreeNode*)calloc(1, sizeof(TreeNode));
	DockData* pDockData = (DockData*)calloc(1, sizeof(DockData));
	if (!pNode || !pDockData)
	{
		free(pNode);
		free(pDockData);
		return NULL;
	}

	DockShell_InitDockData(pDockData, nRole, pszName, nDockSide, iGripPos, dwStyle, bShowCaption);
	pNode->data = pDockData;
	return pNode;
}

static PCWSTR DockShell_GetZoneName(int nDockSide)
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

static TreeNode* DockShell_CreateShellSplitNode(PCWSTR pszName, DWORD dwStyle, int iGripPos, TreeNode* pNode1, TreeNode* pNode2)
{
	TreeNode* pSplitNode = DockShell_CreateNode(
		DOCK_ROLE_SHELL_SPLIT,
		pszName,
		DKS_NONE,
		iGripPos,
		dwStyle,
		FALSE);
	if (!pSplitNode)
	{
		return NULL;
	}

	pSplitNode->node1 = pNode1;
	pSplitNode->node2 = pNode2;
	return pSplitNode;
}

BOOL DockShell_InitRootNode(TreeNode* pRootNode)
{
	if (!pRootNode)
	{
		return FALSE;
	}

	if (!pRootNode->data)
	{
		pRootNode->data = calloc(1, sizeof(DockData));
		if (!pRootNode->data)
		{
			return FALSE;
		}
	}

	DockShell_InitDockData(
		(DockData*)pRootNode->data,
		DOCK_ROLE_ROOT,
		L"Root",
		DKS_NONE,
		64,
		DGA_END | DGP_ABSOLUTE | DGD_VERTICAL,
		FALSE);
	return TRUE;
}

TreeNode* DockShell_CreateRootNode(void)
{
	TreeNode* pRootNode = (TreeNode*)calloc(1, sizeof(TreeNode));
	if (!pRootNode)
	{
		return NULL;
	}

	if (!DockShell_InitRootNode(pRootNode))
	{
		free(pRootNode);
		return NULL;
	}

	return pRootNode;
}

TreeNode* DockShell_CreateZoneNode(int nDockSide)
{
	PCWSTR pszZoneName = DockShell_GetZoneName(nDockSide);
	if (!pszZoneName)
	{
		return NULL;
	}

	return DockShell_CreateNode(
		DOCK_ROLE_ZONE,
		pszZoneName,
		nDockSide,
		200,
		DGA_END | DGP_ABSOLUTE | DGD_VERTICAL,
		FALSE);
}

TreeNode* DockShell_CreatePanelNode(PCWSTR pszName)
{
	return DockShell_CreateNode(
		DOCK_ROLE_PANEL,
		pszName,
		DKS_NONE,
		64,
		DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL,
		TRUE);
}

TreeNode* DockShell_CreateWorkspaceNode(void)
{
	TreeNode* pWorkspaceNode = DockShell_CreateNode(
		DOCK_ROLE_WORKSPACE,
		L"WorkspaceContainer",
		DKS_NONE,
		220,
		DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL,
		FALSE);
	if (!pWorkspaceNode || !pWorkspaceNode->data)
	{
		return pWorkspaceNode;
	}

	((DockData*)pWorkspaceNode->data)->bShowCaption = FALSE;
	return pWorkspaceNode;
}

BOOL DockShell_AppendPanelToZone(TreeNode* pZoneNode, TreeNode* pPanelNode)
{
	if (!pZoneNode || !pPanelNode || !pZoneNode->data)
	{
		return FALSE;
	}

	DockData* pZoneData = (DockData*)pZoneNode->data;
	if (pZoneData->nRole != DOCK_ROLE_ZONE || pZoneData->nDockSide == DKS_NONE)
	{
		return FALSE;
	}

	if (!pZoneNode->node1)
	{
		pZoneNode->node1 = pPanelNode;
		return TRUE;
	}

	TreeNode* pSplitNode = DockShell_CreateNode(
		DOCK_ROLE_ZONE_STACK_SPLIT,
		L"DockShell.ZoneStack",
		DKS_NONE,
		DockLayout_GetZoneStackGrip(pZoneData->nDockSide, 0),
		DockLayout_GetZoneStackStyle(pZoneData->nDockSide),
		FALSE);
	if (!pSplitNode)
	{
		return FALSE;
	}

	pSplitNode->node1 = pZoneNode->node1;
	pSplitNode->node2 = pPanelNode;
	pZoneNode->node1 = pSplitNode;
	return TRUE;
}

BOOL DockShell_BuildMainLayout(
	TreeNode* pRootNode,
	TreeNode* pWorkspaceNode,
	TreeNode* pZoneLeft,
	TreeNode* pZoneRight,
	TreeNode* pZoneTop,
	TreeNode* pZoneBottom,
	const DockShellMetrics* pMetrics)
{
	DockShellMetrics metrics = { 220, 300, 72, 72 };
	TreeNode* pNodeCenterRight;
	TreeNode* pNodeMiddleBand;
	TreeNode* pNodeTopMiddle;
	TreeNode* pNodeShellRoot;

	if (!pRootNode || !pWorkspaceNode || !pZoneLeft || !pZoneRight || !pZoneTop || !pZoneBottom)
	{
		return FALSE;
	}

	if (!DockShell_InitRootNode(pRootNode))
	{
		return FALSE;
	}

	if (pMetrics)
	{
		metrics = *pMetrics;
	}

	pNodeCenterRight = DockShell_CreateShellSplitNode(
		L"DockShell.CenterRight",
		DGA_END | DGP_ABSOLUTE | DGD_HORIZONTAL,
		metrics.rightSize,
		pWorkspaceNode,
		pZoneRight);
	pNodeMiddleBand = DockShell_CreateShellSplitNode(
		L"DockShell.MiddleBand",
		DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL,
		metrics.leftSize,
		pZoneLeft,
		pNodeCenterRight);
	pNodeTopMiddle = DockShell_CreateShellSplitNode(
		L"DockShell.TopMiddle",
		DGA_START | DGP_ABSOLUTE | DGD_VERTICAL,
		metrics.topSize,
		pZoneTop,
		pNodeMiddleBand);
	pNodeShellRoot = DockShell_CreateShellSplitNode(
		L"DockShell.Root",
		DGA_END | DGP_ABSOLUTE | DGD_VERTICAL,
		metrics.bottomSize,
		pNodeTopMiddle,
		pZoneBottom);

	if (!pNodeCenterRight || !pNodeMiddleBand || !pNodeTopMiddle || !pNodeShellRoot)
	{
		return FALSE;
	}

	pRootNode->node1 = pNodeShellRoot;
	pRootNode->node2 = NULL;
	return TRUE;
}

BOOL DockShell_BuildWorkspaceOnlyLayout(TreeNode* pRootNode, TreeNode* pWorkspaceNode)
{
	if (!pRootNode || !pWorkspaceNode)
	{
		return FALSE;
	}

	if (!DockShell_InitRootNode(pRootNode))
	{
		return FALSE;
	}

	pRootNode->node1 = pWorkspaceNode;
	pRootNode->node2 = NULL;
	return TRUE;
}

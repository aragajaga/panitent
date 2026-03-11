#pragma once

#include "dockhost.h"

typedef struct DockShellMetrics
{
	int leftSize;
	int rightSize;
	int topSize;
	int bottomSize;
} DockShellMetrics;

BOOL DockShell_InitRootNode(TreeNode* pRootNode);
TreeNode* DockShell_CreateRootNode(void);
TreeNode* DockShell_CreateZoneNode(int nDockSide);
TreeNode* DockShell_CreatePanelNode(PCWSTR pszName);
TreeNode* DockShell_CreateWorkspaceNode(void);
BOOL DockShell_AppendPanelToZone(TreeNode* pZoneNode, TreeNode* pPanelNode);
BOOL DockShell_BuildMainLayout(
	TreeNode* pRootNode,
	TreeNode* pWorkspaceNode,
	TreeNode* pZoneLeft,
	TreeNode* pZoneRight,
	TreeNode* pZoneTop,
	TreeNode* pZoneBottom,
	const DockShellMetrics* pMetrics);
BOOL DockShell_BuildWorkspaceOnlyLayout(TreeNode* pRootNode, TreeNode* pWorkspaceNode);

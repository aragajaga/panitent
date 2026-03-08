#include "precomp.h"

#include "dockgroup.h"

static TreeNode* DockGroup_FindParent(TreeNode* pRoot, TreeNode* pNode)
{
	if (!pRoot || !pNode)
	{
		return NULL;
	}

	if (pRoot->node1 == pNode || pRoot->node2 == pNode)
	{
		return pRoot;
	}

	TreeNode* pParent = DockGroup_FindParent(pRoot->node1, pNode);
	if (pParent)
	{
		return pParent;
	}

	return DockGroup_FindParent(pRoot->node2, pNode);
}

DockGroupKind DockGroup_GetNodeKind(const TreeNode* pNode)
{
	const DockData* pDockData = pNode ? (const DockData*)pNode->data : NULL;
	if (!pDockData)
	{
		return DOCK_GROUP_NONE;
	}

	switch (pDockData->nRole)
	{
	case DOCK_ROLE_ZONE:
		return DOCK_GROUP_TOOL_PANE;
	case DOCK_ROLE_WORKSPACE:
		return DOCK_GROUP_DOCUMENT_PANE;
	default:
		return DOCK_GROUP_NONE;
	}
}

DockPaneKind DockGroup_GetNodePaneKind(const TreeNode* pNode)
{
	const DockData* pDockData = pNode ? (const DockData*)pNode->data : NULL;
	if (!pDockData)
	{
		return DOCK_PANE_NONE;
	}

	return pDockData->nPaneKind;
}

BOOL DockGroup_NodeContainsPaneKind(const TreeNode* pNode, DockPaneKind nPaneKind)
{
	if (!pNode)
	{
		return FALSE;
	}

	const DockData* pDockData = (const DockData*)pNode->data;
	if (pDockData && pDockData->nPaneKind == nPaneKind)
	{
		return TRUE;
	}

	return DockGroup_NodeContainsPaneKind(pNode->node1, nPaneKind) ||
		DockGroup_NodeContainsPaneKind(pNode->node2, nPaneKind);
}

TreeNode* DockGroup_FindOwningGroup(TreeNode* pRoot, TreeNode* pNode)
{
	if (!pRoot || !pNode)
	{
		return NULL;
	}

	TreeNode* pCurrent = pNode;
	while (pCurrent)
	{
		DockGroupKind nGroupKind = DockGroup_GetNodeKind(pCurrent);
		if (nGroupKind != DOCK_GROUP_NONE)
		{
			return pCurrent;
		}

		if (pCurrent == pRoot)
		{
			break;
		}

		pCurrent = DockGroup_FindParent(pRoot, pCurrent);
	}

	return NULL;
}

BOOL DockGroup_CanTabIntoGroup(DockGroupKind nGroupKind, DockPaneKind nPaneKind)
{
	switch (nGroupKind)
	{
	case DOCK_GROUP_TOOL_PANE:
		return nPaneKind == DOCK_PANE_TOOL;
	case DOCK_GROUP_DOCUMENT_PANE:
		return nPaneKind == DOCK_PANE_DOCUMENT;
	default:
		return FALSE;
	}
}

BOOL DockGroup_CanSplitGroup(DockGroupKind nGroupKind, DockPaneKind nPaneKind)
{
	switch (nGroupKind)
	{
	case DOCK_GROUP_TOOL_PANE:
		return nPaneKind == DOCK_PANE_TOOL;
	case DOCK_GROUP_DOCUMENT_PANE:
		return nPaneKind == DOCK_PANE_DOCUMENT;
	default:
		return FALSE;
	}
}

#include "precomp.h"

#include <stdint.h>

#include "dockmodelbuild.h"

static HWND DockModelBuild_AllocateSyntheticHwnd(uintptr_t* pNextHandle)
{
	HWND hWnd = (HWND)(INT_PTR)(*pNextHandle);
	(*pNextHandle)++;
	return hWnd;
}

static TreeNode* DockModelBuild_FindByName(TreeNode* pNode, PCWSTR pszName)
{
	if (!pNode || !pszName || !pszName[0] || !pNode->data)
	{
		return NULL;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (wcscmp(pDockData->lpszName, pszName) == 0)
	{
		return pNode;
	}

	TreeNode* pFound = DockModelBuild_FindByName(pNode->node1, pszName);
	if (pFound)
	{
		return pFound;
	}

	return DockModelBuild_FindByName(pNode->node2, pszName);
}

static TreeNode* DockModelBuild_BuildNode(const DockModelNode* pModelNode, uintptr_t* pNextHandle)
{
	if (!pModelNode || !pNextHandle)
	{
		return NULL;
	}

	TreeNode* pNode = (TreeNode*)calloc(1, sizeof(TreeNode));
	DockData* pDockData = (DockData*)calloc(1, sizeof(DockData));
	if (!pNode || !pDockData)
	{
		free(pNode);
		free(pDockData);
		return NULL;
	}

	pNode->data = pDockData;
	pDockData->uModelNodeId = pModelNode->uNodeId;
	pDockData->nRole = pModelNode->nRole;
	pDockData->nPaneKind = pModelNode->nPaneKind;
	pDockData->nDockSide = pModelNode->nDockSide;
	pDockData->dwStyle = pModelNode->dwStyle;
	pDockData->fGripPos = pModelNode->fGripPos;
	pDockData->iGripPos = pModelNode->iGripPos;
	pDockData->bShowCaption = pModelNode->bShowCaption;
	pDockData->bCollapsed = pModelNode->bCollapsed;
	wcscpy_s(pDockData->lpszName, ARRAYSIZE(pDockData->lpszName), pModelNode->szName);
	wcscpy_s(pDockData->lpszCaption, ARRAYSIZE(pDockData->lpszCaption), pModelNode->szCaption);

	if (pDockData->nPaneKind != DOCK_PANE_NONE ||
		pDockData->nRole == DOCK_ROLE_WORKSPACE)
	{
		pDockData->hWnd = DockModelBuild_AllocateSyntheticHwnd(pNextHandle);
	}

	pNode->node1 = DockModelBuild_BuildNode(pModelNode->pChild1, pNextHandle);
	pNode->node2 = DockModelBuild_BuildNode(pModelNode->pChild2, pNextHandle);

	if (pModelNode->szActiveTabName[0] != L'\0')
	{
		TreeNode* pActiveNode = DockModelBuild_FindByName(pNode, pModelNode->szActiveTabName);
		if (pActiveNode && pActiveNode->data)
		{
			pDockData->hWndActiveTab = ((DockData*)pActiveNode->data)->hWnd;
		}
	}

	return pNode;
}

TreeNode* DockModelBuildTree(const DockModelNode* pRootModel)
{
	uintptr_t nextHandle = 1;
	return DockModelBuild_BuildNode(pRootModel, &nextHandle);
}

void DockModelBuildDestroyTree(TreeNode* pRootNode)
{
	if (!pRootNode)
	{
		return;
	}

	DockModelBuildDestroyTree(pRootNode->node1);
	DockModelBuildDestroyTree(pRootNode->node2);
	free(pRootNode->data);
	free(pRootNode);
}

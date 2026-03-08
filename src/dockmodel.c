#include "precomp.h"

#include "dockmodel.h"

static const TreeNode* DockModel_FindByHWND(const TreeNode* pNode, HWND hWnd)
{
	if (!pNode || !hWnd)
	{
		return NULL;
	}

	const DockData* pDockData = (const DockData*)pNode->data;
	if (pDockData && pDockData->hWnd == hWnd)
	{
		return pNode;
	}

	const TreeNode* pFound = DockModel_FindByHWND(pNode->node1, hWnd);
	if (pFound)
	{
		return pFound;
	}

	return DockModel_FindByHWND(pNode->node2, hWnd);
}

static void DockModel_CopyWString(WCHAR* pszDest, size_t cchDest, PCWSTR pszSource)
{
	if (!pszDest || cchDest == 0)
	{
		return;
	}

	pszDest[0] = L'\0';
	if (!pszSource)
	{
		return;
	}

	wcscpy_s(pszDest, cchDest, pszSource);
}

static void DockModel_ResolveActiveTabName(const TreeNode* pRootNode, const DockData* pDockData, WCHAR* pszName, size_t cchName)
{
	if (!pszName || cchName == 0)
	{
		return;
	}

	pszName[0] = L'\0';
	if (!pRootNode || !pDockData || !pDockData->hWndActiveTab)
	{
		return;
	}

	const TreeNode* pActiveNode = DockModel_FindByHWND(pRootNode, pDockData->hWndActiveTab);
	if (!pActiveNode || !pActiveNode->data)
	{
		return;
	}

	const DockData* pActiveData = (const DockData*)pActiveNode->data;
	if (pActiveData->lpszName[0] != L'\0')
	{
		DockModel_CopyWString(pszName, cchName, pActiveData->lpszName);
		return;
	}

	DockModel_CopyWString(pszName, cchName, pActiveData->lpszCaption);
}

static DockModelNode* DockModel_CaptureNode(const TreeNode* pRootNode, const TreeNode* pNode)
{
	DockModelNode* pModelNode;
	const DockData* pDockData;
	if (!pNode || !pNode->data)
	{
		return NULL;
	}

	pDockData = (const DockData*)pNode->data;
	pModelNode = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	if (!pModelNode)
	{
		return NULL;
	}

	pModelNode->nRole = pDockData->nRole;
	pModelNode->nPaneKind = pDockData->nPaneKind;
	pModelNode->nDockSide = pDockData->nDockSide;
	pModelNode->dwStyle = pDockData->dwStyle;
	pModelNode->fGripPos = pDockData->fGripPos;
	pModelNode->iGripPos = pDockData->iGripPos;
	pModelNode->bShowCaption = pDockData->bShowCaption;
	pModelNode->bCollapsed = pDockData->bCollapsed;
	DockModel_CopyWString(pModelNode->szName, ARRAYSIZE(pModelNode->szName), pDockData->lpszName);
	DockModel_CopyWString(pModelNode->szCaption, ARRAYSIZE(pModelNode->szCaption), pDockData->lpszCaption);
	DockModel_ResolveActiveTabName(pRootNode, pDockData, pModelNode->szActiveTabName, ARRAYSIZE(pModelNode->szActiveTabName));

	pModelNode->pChild1 = DockModel_CaptureNode(pRootNode, pNode->node1);
	pModelNode->pChild2 = DockModel_CaptureNode(pRootNode, pNode->node2);
	return pModelNode;
}

DockModelNode* DockModel_CaptureTree(const TreeNode* pRootNode)
{
	if (!pRootNode)
	{
		return NULL;
	}

	return DockModel_CaptureNode(pRootNode, pRootNode);
}

DockModelNode* DockModel_CaptureHostLayout(const DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		return NULL;
	}

	return DockModel_CaptureTree(pDockHostWindow->pRoot_);
}

void DockModel_Destroy(DockModelNode* pNode)
{
	if (!pNode)
	{
		return;
	}

	DockModel_Destroy(pNode->pChild1);
	DockModel_Destroy(pNode->pChild2);
	free(pNode);
}

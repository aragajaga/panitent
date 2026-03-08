#include "precomp.h"

#include "docklayoutpersist.h"

#include "dockhost.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockmodelio.h"
#include "dockmodelvalidate.h"
#include "dockviewfactory.h"
#include "panitentapp.h"
#include "shell/pathutil.h"

typedef struct DockLayoutHandleRemap DockLayoutHandleRemap;
struct DockLayoutHandleRemap
{
	HWND hOld;
	HWND hNew;
};

typedef struct DockLayoutRestoreContext DockLayoutRestoreContext;
struct DockLayoutRestoreContext
{
	PanitentApp* pPanitentApp;
	DockHostWindow* pDockHostWindow;
	DockLayoutHandleRemap remaps[64];
	int nRemaps;
};

static void DockLayoutPersist_RecordHandleRemap(DockLayoutRestoreContext* pContext, HWND hOld, HWND hNew)
{
	if (!pContext || !hOld || !hNew || pContext->nRemaps >= ARRAYSIZE(pContext->remaps))
	{
		return;
	}

	pContext->remaps[pContext->nRemaps].hOld = hOld;
	pContext->remaps[pContext->nRemaps].hNew = hNew;
	pContext->nRemaps++;
}

static HWND DockLayoutPersist_MapHandle(const DockLayoutRestoreContext* pContext, HWND hOld)
{
	if (!pContext || !hOld)
	{
		return NULL;
	}

	for (int i = 0; i < pContext->nRemaps; ++i)
	{
		if (pContext->remaps[i].hOld == hOld)
		{
			return pContext->remaps[i].hNew;
		}
	}

	return NULL;
}

static Window* DockLayoutPersist_CreateWindowForNode(DockLayoutRestoreContext* pContext, DockData* pDockData)
{
	if (!pContext || !pDockData)
	{
		return NULL;
	}

	return PanitentDockViewFactory_CreateWindow(
		pContext->pPanitentApp,
		PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName));
}

static BOOL DockLayoutPersist_AttachWindowsRecursive(DockLayoutRestoreContext* pContext, TreeNode* pNode)
{
	if (!pNode || !pNode->data)
	{
		return TRUE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData->nRole == DOCK_ROLE_WORKSPACE || pDockData->nRole == DOCK_ROLE_PANEL)
	{
		HWND hOld = pDockData->hWnd;
		Window* pWindow = DockLayoutPersist_CreateWindowForNode(pContext, pDockData);
		if (!pWindow)
		{
			return FALSE;
		}

		if (!Window_CreateWindow(pWindow, NULL))
		{
			return FALSE;
		}

		DockData_PinWindow(pContext->pDockHostWindow, pDockData, pWindow);
		if (pDockData->nRole == DOCK_ROLE_WORKSPACE)
		{
			pDockData->bShowCaption = FALSE;
		}
		DockLayoutPersist_RecordHandleRemap(pContext, hOld, pDockData->hWnd);
	}

	return DockLayoutPersist_AttachWindowsRecursive(pContext, pNode->node1) &&
		DockLayoutPersist_AttachWindowsRecursive(pContext, pNode->node2);
}

static void DockLayoutPersist_RemapActiveTabsRecursive(const DockLayoutRestoreContext* pContext, TreeNode* pNode)
{
	if (!pNode || !pNode->data)
	{
		return;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData->hWndActiveTab)
	{
		HWND hMapped = DockLayoutPersist_MapHandle(pContext, pDockData->hWndActiveTab);
		if (hMapped)
		{
			pDockData->hWndActiveTab = hMapped;
		}
	}

	DockLayoutPersist_RemapActiveTabsRecursive(pContext, pNode->node1);
	DockLayoutPersist_RemapActiveTabsRecursive(pContext, pNode->node2);
}

BOOL PanitentDockLayout_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	DockModelNode* pModelRoot;
	PTSTR pszDockLayoutFilePath = NULL;
	BOOL bResult;

	UNREFERENCED_PARAMETER(pPanitentApp);

	if (!pDockHostWindow)
	{
		return FALSE;
	}

	pModelRoot = DockModel_CaptureHostLayout(pDockHostWindow);
	if (!pModelRoot)
	{
		return FALSE;
	}

	GetDockLayoutFilePath(&pszDockLayoutFilePath);
	if (!pszDockLayoutFilePath)
	{
		DockModel_Destroy(pModelRoot);
		return FALSE;
	}

	bResult = DockModelIO_SaveToFile(pModelRoot, pszDockLayoutFilePath);
	free(pszDockLayoutFilePath);
	DockModel_Destroy(pModelRoot);
	return bResult;
}

BOOL PanitentDockLayout_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const RECT* pDockHostRect)
{
	PTSTR pszDockLayoutFilePath = NULL;
	DockModelNode* pModelRoot;
	TreeNode* pRootNode;
	DockLayoutRestoreContext context = { 0 };

	if (!pPanitentApp || !pDockHostWindow || !pDockHostRect)
	{
		return FALSE;
	}

	GetDockLayoutFilePath(&pszDockLayoutFilePath);
	if (!pszDockLayoutFilePath)
	{
		return FALSE;
	}

	pModelRoot = DockModelIO_LoadFromFile(pszDockLayoutFilePath);
	free(pszDockLayoutFilePath);
	if (!pModelRoot)
	{
		return FALSE;
	}

	if (!DockModelValidateAndRepairMainLayout(&pModelRoot, NULL))
	{
		DockModel_Destroy(pModelRoot);
		return FALSE;
	}

	pRootNode = DockModelBuildTree(pModelRoot);
	DockModel_Destroy(pModelRoot);
	if (!pRootNode || !pRootNode->data)
	{
		DockModelBuildDestroyTree(pRootNode);
		return FALSE;
	}

	((DockData*)pRootNode->data)->rc = *pDockHostRect;

	pPanitentApp->m_pWorkspaceContainer = NULL;
	pPanitentApp->m_pPaletteWindow = NULL;
	PanitentApp_SetOptionBar(pPanitentApp, NULL);

	context.pPanitentApp = pPanitentApp;
	context.pDockHostWindow = pDockHostWindow;
	if (!DockLayoutPersist_AttachWindowsRecursive(&context, pRootNode))
	{
		DockModelBuildDestroyTree(pRootNode);
		return FALSE;
	}

	DockLayoutPersist_RemapActiveTabsRecursive(&context, pRootNode);
	DockHostWindow_SetRoot(pDockHostWindow, pRootNode);
	return pPanitentApp->m_pWorkspaceContainer != NULL;
}

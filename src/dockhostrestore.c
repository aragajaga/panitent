#include "precomp.h"

#include "dockhostrestore.h"

#include "dockhost.h"
#include "dockviewcatalog.h"
#include "dockviewfactory.h"

typedef struct DockHostRestoreHandleRemap
{
	HWND hOld;
	HWND hNew;
} DockHostRestoreHandleRemap;

typedef struct DockHostRestoreContext
{
	PanitentApp* pPanitentApp;
	DockHostWindow* pDockHostWindow;
	DockHostRestoreHandleRemap remaps[64];
	int nRemaps;
	BOOL bHasWorkspace;
} DockHostRestoreContext;

static void DockHostRestore_RecordHandleRemap(DockHostRestoreContext* pContext, HWND hOld, HWND hNew)
{
	if (!pContext || !hOld || !hNew || pContext->nRemaps >= ARRAYSIZE(pContext->remaps))
	{
		return;
	}

	pContext->remaps[pContext->nRemaps].hOld = hOld;
	pContext->remaps[pContext->nRemaps].hNew = hNew;
	pContext->nRemaps++;
}

static HWND DockHostRestore_MapHandle(const DockHostRestoreContext* pContext, HWND hOld)
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

static BOOL DockHostRestore_AttachWindowsRecursive(DockHostRestoreContext* pContext, TreeNode* pNode)
{
	if (!pContext || !pNode || !pNode->data)
	{
		return TRUE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	PanitentDockViewId nViewId = PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName);
	if (nViewId != PNT_DOCK_VIEW_NONE)
	{
		HWND hOld = pDockData->hWnd;
		Window* pWindow = PanitentDockViewFactory_CreateWindow(pContext->pPanitentApp, nViewId);
		if (!pWindow || !Window_CreateWindow(pWindow, NULL))
		{
			return FALSE;
		}

		DockData_PinWindow(pContext->pDockHostWindow, pDockData, pWindow);
		if (pDockData->nRole == DOCK_ROLE_WORKSPACE)
		{
			pDockData->bShowCaption = FALSE;
			pContext->bHasWorkspace = TRUE;
		}
		DockHostRestore_RecordHandleRemap(pContext, hOld, pDockData->hWnd);
	}

	return DockHostRestore_AttachWindowsRecursive(pContext, pNode->node1) &&
		DockHostRestore_AttachWindowsRecursive(pContext, pNode->node2);
}

static void DockHostRestore_RemapActiveTabsRecursive(const DockHostRestoreContext* pContext, TreeNode* pNode)
{
	if (!pContext || !pNode || !pNode->data)
	{
		return;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData->hWndActiveTab)
	{
		HWND hMapped = DockHostRestore_MapHandle(pContext, pDockData->hWndActiveTab);
		if (hMapped)
		{
			pDockData->hWndActiveTab = hMapped;
		}
	}

	DockHostRestore_RemapActiveTabsRecursive(pContext, pNode->node1);
	DockHostRestore_RemapActiveTabsRecursive(pContext, pNode->node2);
}

BOOL PanitentDockHostRestoreAttachKnownViews(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pRootNode,
	BOOL* pbHasWorkspace)
{
	DockHostRestoreContext context = { 0 };
	context.pPanitentApp = pPanitentApp;
	context.pDockHostWindow = pDockHostWindow;
	if (!DockHostRestore_AttachWindowsRecursive(&context, pRootNode))
	{
		return FALSE;
	}

	DockHostRestore_RemapActiveTabsRecursive(&context, pRootNode);
	if (pbHasWorkspace)
	{
		*pbHasWorkspace = context.bHasWorkspace;
	}
	return TRUE;
}

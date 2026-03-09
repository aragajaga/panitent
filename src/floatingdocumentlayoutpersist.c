#include "precomp.h"

#include "floatingdocumentlayoutpersist.h"

#include "dockhost.h"
#include "dockhostrestore.h"
#include "floatingdocumenthost.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "dockmodelbuild.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"
#include "workspacecontainer.h"

typedef struct FloatingDocumentLayoutCaptureContext
{
	FloatingDocumentLayoutModel* pModel;
} FloatingDocumentLayoutCaptureContext;

typedef struct FloatingDocumentLayoutApplyContext
{
	HWND hWorkspaceHwnds[32];
	int nWorkspaceCount;
	int iNextWorkspace;
} FloatingDocumentLayoutApplyContext;

static BOOL FloatingDocumentLayout_OnPinnedWindowCapture(
	HWND hWnd,
	FloatingWindowContainer* pFloatingWindowContainer,
	void* pUserData)
{
	FloatingDocumentLayoutCaptureContext* pContext = (FloatingDocumentLayoutCaptureContext*)pUserData;
	if (!pContext || !pContext->pModel || !IsWindow(hWnd))
	{
		return TRUE;
	}

	if (pContext->pModel->nEntryCount >= ARRAYSIZE(pContext->pModel->entries))
	{
		return TRUE;
	}

	FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(pFloatingWindowContainer->hWndChild);
	if (nChildKind != FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE &&
		nChildKind != FLOAT_DOCK_CHILD_DOCUMENT_HOST)
	{
		return TRUE;
	}

	FloatingDocumentLayoutEntry* pEntry = &pContext->pModel->entries[pContext->pModel->nEntryCount];
	memset(pEntry, 0, sizeof(*pEntry));
	if (!FloatingDocumentHost_CapturePinnedWindowState(
		hWnd,
		pFloatingWindowContainer,
		&pEntry->rcWindow,
		&pEntry->pLayoutModel,
		NULL,
		0,
		NULL))
	{
		return TRUE;
	}

	pContext->pModel->nEntryCount++;
	return TRUE;
}

BOOL PanitentFloatingDocumentLayout_CaptureModel(PanitentApp* pPanitentApp, FloatingDocumentLayoutModel* pModel)
{
	UNREFERENCED_PARAMETER(pPanitentApp);

	if (!pModel)
	{
		return FALSE;
	}

	memset(pModel, 0, sizeof(*pModel));
	FloatingDocumentLayoutCaptureContext context = { pModel };
	FloatingDocumentHost_ForEachPinnedWindow(FloatingDocumentLayout_OnPinnedWindowCapture, &context);
	return TRUE;
}

static BOOL WindowLayoutManager_CollectFloatingDocumentWorkspaces(HWND hWndChild, HWND* pWorkspaceHwnds, int cWorkspaceHwnds, int* pnCount)
{
	int nFound = FloatingChildHost_CollectDocumentWorkspaceHwnds(hWndChild, pWorkspaceHwnds, cWorkspaceHwnds);
	if (pnCount)
	{
		*pnCount = nFound;
	}
	return nFound > 0;
}

static BOOL FloatingDocumentLayout_OnPinnedWindowCollectLive(
	HWND hWnd,
	FloatingWindowContainer* pFloatingWindowContainer,
	void* pUserData)
{
	FloatingDocumentLayoutApplyContext* pContext = (FloatingDocumentLayoutApplyContext*)pUserData;
	if (!pContext || !IsWindow(hWnd))
	{
		return TRUE;
	}

	HWND hWndChild = pFloatingWindowContainer->hWndChild;
	if (!hWndChild || !IsWindow(hWndChild))
	{
		return TRUE;
	}

	HWND hWorkspaceHwnds[16] = { 0 };
	int nWorkspaceCount = 0;
	if (WindowLayoutManager_CollectFloatingDocumentWorkspaces(hWndChild, hWorkspaceHwnds, ARRAYSIZE(hWorkspaceHwnds), &nWorkspaceCount))
	{
		for (int i = 0; i < nWorkspaceCount && pContext->nWorkspaceCount < ARRAYSIZE(pContext->hWorkspaceHwnds); ++i)
		{
			SetParent(hWorkspaceHwnds[i], HWND_DESKTOP);
			pContext->hWorkspaceHwnds[pContext->nWorkspaceCount++] = hWorkspaceHwnds[i];
		}
	}

	DestroyWindow(hWnd);
	return TRUE;
}

static Window* FloatingDocumentLayout_ResolveWorkspace(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pNode,
	DockData* pDockData,
	PanitentDockViewId nViewId,
	void* pUserData)
{
	UNREFERENCED_PARAMETER(pPanitentApp);
	UNREFERENCED_PARAMETER(pDockHostWindow);
	UNREFERENCED_PARAMETER(pNode);
	UNREFERENCED_PARAMETER(pDockData);

	FloatingDocumentLayoutApplyContext* pContext = (FloatingDocumentLayoutApplyContext*)pUserData;
	if (nViewId != PNT_DOCK_VIEW_WORKSPACE || !pContext)
	{
		return NULL;
	}

	if (pContext->iNextWorkspace < pContext->nWorkspaceCount)
	{
		HWND hWndWorkspace = pContext->hWorkspaceHwnds[pContext->iNextWorkspace++];
		return (Window*)WindowMap_Get(hWndWorkspace);
	}

	WorkspaceContainer* pWorkspace = WorkspaceContainer_Create();
	return (Window*)pWorkspace;
}

BOOL PanitentFloatingDocumentLayout_RestoreModel(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel)
{
	if (!pPanitentApp || !pDockHostWindow || !pModel)
	{
		return FALSE;
	}

	FloatingDocumentLayoutApplyContext context = { 0 };
	FloatingDocumentHost_ForEachPinnedWindow(FloatingDocumentLayout_OnPinnedWindowCollectLive, &context);

	BOOL bRestoredAny = FALSE;
	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		const FloatingDocumentLayoutEntry* pEntry = &pModel->entries[i];
		if (!pEntry->pLayoutModel)
		{
			continue;
		}

		DockHostWindow* pFloatingDockHost = NULL;
		HWND hWndFloating = NULL;
		BOOL bHasWorkspace = FALSE;
		if (!FloatingDocumentHost_RestorePinnedDockHost(
			pPanitentApp,
			pDockHostWindow,
			&pEntry->rcWindow,
			pEntry->pLayoutModel,
			FloatingDocumentLayout_ResolveWorkspace,
			&context,
			NULL,
			NULL,
			&bHasWorkspace,
			&pFloatingDockHost,
			&hWndFloating))
		{
			continue;
		}
		bRestoredAny = TRUE;
	}

	if (context.iNextWorkspace < context.nWorkspaceCount)
	{
		WorkspaceContainer* pMainWorkspace = PanitentApp_GetWorkspaceContainer(pPanitentApp);
		for (int i = context.iNextWorkspace; i < context.nWorkspaceCount; ++i)
		{
			WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(context.hWorkspaceHwnds[i]);
			if (pWorkspace && pMainWorkspace)
			{
				WorkspaceContainer_MoveAllViewportsTo(pWorkspace, pMainWorkspace);
			}
			if (context.hWorkspaceHwnds[i] && IsWindow(context.hWorkspaceHwnds[i]))
			{
				DestroyWindow(context.hWorkspaceHwnds[i]);
			}
		}
	}

	return bRestoredAny || pModel->nEntryCount == 0;
}

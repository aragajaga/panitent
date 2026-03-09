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
	FloatingDocumentWorkspaceReuseContext reuse;
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
	FloatingDocumentHost_CollectLiveWorkspaces(&context.reuse);

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
			FloatingDocumentHost_ResolveReusedWorkspace,
			&context.reuse,
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

	FloatingDocumentHost_DisposeUnusedReusedWorkspaces(pPanitentApp, &context.reuse);

	return bRestoredAny || pModel->nEntryCount == 0;
}

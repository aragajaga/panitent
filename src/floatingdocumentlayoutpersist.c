#include "precomp.h"

#include "floatingdocumentlayoutpersist.h"

#include "dockhost.h"
#include "dockhostrestore.h"
#include "floatingdocumentreuse.h"
#include "floatingdocumentrestore.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "dockmodelbuild.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"
#include "workspacecontainer.h"

static FnFloatingDocumentLayoutRestoreEntryTestHook g_pFloatingDocumentLayoutRestoreEntryTestHook = NULL;

typedef struct FloatingDocumentLayoutCaptureContext
{
	FloatingDocumentLayoutModel* pModel;
} FloatingDocumentLayoutCaptureContext;

typedef struct FloatingDocumentLayoutApplyContext
{
	FloatingDocumentWorkspaceReuseContext reuse;
} FloatingDocumentLayoutApplyContext;

static BOOL FloatingDocumentLayout_RestoreEntries(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel,
	BOOL bUseRestoreHook,
	BOOL* pbRestoredAny,
	int* pnAttempted,
	int* pnRestored)
{
	if (pbRestoredAny)
	{
		*pbRestoredAny = FALSE;
	}
	if (pnAttempted)
	{
		*pnAttempted = 0;
	}
	if (pnRestored)
	{
		*pnRestored = 0;
	}

	FloatingDocumentLayoutApplyContext context = { 0 };
	FloatingDocumentHost_PrepareWorkspaceReuse(&context.reuse, FALSE);

	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		const FloatingDocumentLayoutEntry* pEntry = &pModel->entries[i];
		if (!pEntry->pLayoutModel)
		{
			continue;
		}

		if (pnAttempted)
		{
			(*pnAttempted)++;
		}
		if (bUseRestoreHook &&
			g_pFloatingDocumentLayoutRestoreEntryTestHook &&
			!g_pFloatingDocumentLayoutRestoreEntryTestHook(pEntry))
		{
			FloatingDocumentHost_DisposeUnusedReusedWorkspaces(pPanitentApp, &context.reuse);
			return FALSE;
		}

		DockHostWindow* pFloatingDockHost = NULL;
		HWND hWndFloating = NULL;
		BOOL bHasWorkspace = FALSE;
		if (!FloatingDocumentHost_RestorePinnedDockHostWithReuse(
			pPanitentApp,
			pDockHostWindow,
			&pEntry->rcWindow,
			pEntry->pLayoutModel,
			&context.reuse,
			NULL,
			NULL,
			&bHasWorkspace,
			&pFloatingDockHost,
			&hWndFloating))
		{
			continue;
		}

		if (pbRestoredAny)
		{
			*pbRestoredAny = TRUE;
		}
		if (pnRestored)
		{
			(*pnRestored)++;
		}
	}

	FloatingDocumentHost_DisposeUnusedReusedWorkspaces(pPanitentApp, &context.reuse);
	return TRUE;
}

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

void PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(FnFloatingDocumentLayoutRestoreEntryTestHook pfnHook)
{
	g_pFloatingDocumentLayoutRestoreEntryTestHook = pfnHook;
}

BOOL PanitentFloatingDocumentLayout_RestoreModel(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel)
{
	return PanitentFloatingDocumentLayout_RestoreModelEx(
		pPanitentApp,
		pDockHostWindow,
		pModel,
		FALSE);
}

BOOL PanitentFloatingDocumentLayout_RestoreModelEx(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel,
	BOOL bRequireAllEntries)
{
	if (!pPanitentApp || !pDockHostWindow || !pModel)
	{
		return FALSE;
	}

	FloatingDocumentLayoutModel rollbackModel = { 0 };
	if (bRequireAllEntries)
	{
		PanitentFloatingDocumentLayout_CaptureModel(pPanitentApp, &rollbackModel);
	}

	BOOL bRestoredAny = FALSE;
	int nAttempted = 0;
	int nRestored = 0;
	BOOL bCompleted = FloatingDocumentLayout_RestoreEntries(
		pPanitentApp,
		pDockHostWindow,
		pModel,
		TRUE,
		&bRestoredAny,
		&nAttempted,
		&nRestored);

	if (bRequireAllEntries && (!bCompleted || nRestored != nAttempted))
	{
		FloatingDocumentLayout_RestoreEntries(
			pPanitentApp,
			pDockHostWindow,
			&rollbackModel,
			FALSE,
			NULL,
			NULL,
			NULL);
		FloatingDocumentLayoutModel_Destroy(&rollbackModel);
		return FALSE;
	}

	FloatingDocumentLayoutModel_Destroy(&rollbackModel);

	if (bRequireAllEntries)
	{
		return nRestored == nAttempted;
	}

	return bCompleted && (bRestoredAny || pModel->nEntryCount == 0);
}

#include "precomp.h"

#include "dockfloatingpersist.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "dockgroup.h"
#include "dockhostrestore.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockviewfactory.h"
#include "persistfile.h"
#include "floatingtoolhost.h"
#include "floatingwindowcontainer.h"
#include "floatingchildhost.h"
#include "shell/pathutil.h"

typedef struct DockFloatingPersistCollectContext
{
	DockFloatingLayoutFileModel* pModel;
} DockFloatingPersistCollectContext;

static FnDockFloatingRestoreEntryTestHook g_pDockFloatingRestoreEntryTestHook = NULL;

void PanitentDockFloating_SetRestoreEntryTestHook(FnDockFloatingRestoreEntryTestHook pfnHook)
{
	g_pDockFloatingRestoreEntryTestHook = pfnHook;
}

static BOOL DockFloatingPersist_MainHostHasView(TreeNode* pNode, PanitentDockViewId nViewId)
{
	if (!pNode || nViewId == PNT_DOCK_VIEW_NONE)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	PanitentDockViewId nNodeViewId = pDockData ?
		(pDockData->nViewId != PNT_DOCK_VIEW_NONE ?
			pDockData->nViewId :
			PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName)) :
		PNT_DOCK_VIEW_NONE;
	if (nNodeViewId == nViewId)
	{
		return TRUE;
	}

	return DockFloatingPersist_MainHostHasView(pNode->node1, nViewId) ||
		DockFloatingPersist_MainHostHasView(pNode->node2, nViewId);
}

static BOOL CALLBACK DockFloatingPersist_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	DockFloatingPersistCollectContext* pContext = (DockFloatingPersistCollectContext*)lParam;
	FloatingWindowContainer* pFloatingWindowContainer = NULL;
	if (!pContext || !pContext->pModel || !IsWindow(hWnd))
	{
		return TRUE;
	}

	if (!FloatingToolHost_IsPinnedFloatingWindow(hWnd, &pFloatingWindowContainer))
	{
		return TRUE;
	}

	if (pContext->pModel->nEntries >= ARRAYSIZE(pContext->pModel->entries))
	{
		return TRUE;
	}

	DockFloatingLayoutEntry* pEntry = &pContext->pModel->entries[pContext->pModel->nEntries];
	if (!FloatingToolHost_CapturePinnedWindowState(hWnd, pFloatingWindowContainer, pEntry))
	{
		return TRUE;
	}

	pContext->pModel->nEntries++;

	return TRUE;
}

BOOL PanitentDockFloating_CaptureModel(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, DockFloatingLayoutFileModel* pModel)
{
	UNREFERENCED_PARAMETER(pPanitentApp);
	UNREFERENCED_PARAMETER(pDockHostWindow);

	if (!pModel)
	{
		return FALSE;
	}

	memset(pModel, 0, sizeof(*pModel));
	DockFloatingPersistCollectContext context = { pModel };
	EnumWindows(DockFloatingPersist_EnumWindowsProc, (LPARAM)&context);
	return TRUE;
}

BOOL PanitentDockFloating_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	PTSTR pszFilePath = NULL;
	GetDockFloatingFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bResult = PanitentDockFloating_SaveToFilePath(pPanitentApp, pDockHostWindow, pszFilePath);
	free(pszFilePath);
	return bResult;
}

BOOL PanitentDockFloating_SaveToFilePath(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, PCWSTR pszFilePath)
{
	DockFloatingLayoutFileModel model = { 0 };
	if (!pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}
	if (!PanitentDockFloating_CaptureModel(pPanitentApp, pDockHostWindow, &model))
	{
		return FALSE;
	}

	BOOL bResult = DockFloatingLayout_SaveToFile(&model, pszFilePath);
	DockFloatingLayout_Destroy(&model);
	return bResult;
}

BOOL PanitentDockFloating_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	PTSTR pszFilePath = NULL;
	GetDockFloatingFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bResult = PanitentDockFloating_RestoreFromFilePath(pPanitentApp, pDockHostWindow, pszFilePath);
	free(pszFilePath);
	return bResult;
}

BOOL PanitentDockFloating_RestoreFromFilePath(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, PCWSTR pszFilePath)
{
	DockFloatingLayoutFileModel model = { 0 };
	if (!pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
	BOOL bLoaded = DockFloatingLayout_LoadFromFileEx(pszFilePath, &model, &loadStatus);
	if (!bLoaded && loadStatus == PERSIST_LOAD_INVALID_FORMAT)
	{
		PersistFile_QuarantineInvalid(pszFilePath);
	}
	if (!bLoaded)
	{
		return FALSE;
	}

	BOOL bResult = PanitentDockFloating_RestoreModel(pPanitentApp, pDockHostWindow, &model);
	DockFloatingLayout_Destroy(&model);
	return bResult;
}

BOOL PanitentDockFloating_RestoreModel(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const DockFloatingLayoutFileModel* pModel)
{
	return PanitentDockFloating_RestoreModelEx(pPanitentApp, pDockHostWindow, pModel, FALSE);
}

BOOL PanitentDockFloating_RestoreModelEx(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const DockFloatingLayoutFileModel* pModel, BOOL bRequireAllEntries)
{
	if (!pModel)
	{
		return FALSE;
	}

	FloatingToolHost_DestroyExistingPinnedWindows();

	BOOL bRestoredAny = FALSE;
	int nAttempted = 0;
	int nRestored = 0;
	for (int i = 0; i < pModel->nEntries; ++i)
	{
		const DockFloatingLayoutEntry* pEntry = &pModel->entries[i];
		nAttempted++;
		if (g_pDockFloatingRestoreEntryTestHook &&
			!g_pDockFloatingRestoreEntryTestHook(pEntry))
		{
			return FALSE;
		}

		if (pEntry->nChildKind == FLOAT_DOCK_CHILD_TOOL_PANEL &&
			DockFloatingPersist_MainHostHasView(DockHostWindow_GetRoot(pDockHostWindow), pEntry->nViewId))
		{
			continue;
		}

		if (!FloatingToolHost_RestoreEntry(pPanitentApp, pDockHostWindow, pEntry, NULL))
		{
			continue;
		}
		bRestoredAny = TRUE;
		nRestored++;
	}

	if (bRequireAllEntries)
	{
		return nRestored == nAttempted;
	}

	return bRestoredAny || pModel->nEntries == 0;
}

#include "precomp.h"

#include "floatingdocumentsessionpersist.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "document.h"
#include "documentrecovery.h"
#include "dockhost.h"
#include "dockhostrestore.h"
#include "floatingdocumenthost.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "documentsessionworkspace.h"
#include "persistfile.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "floatingdocumentsessionmodel.h"
#include "panitentapp.h"
#include "recoverystorepersist.h"
#include "shell/pathutil.h"
#include "viewport.h"
#include "workspacecontainer.h"

typedef struct FloatingDocumentPersistCollectContext
{
	FloatingDocumentSessionModel* pModel;
} FloatingDocumentPersistCollectContext;

typedef struct FloatingDocumentRestoreContext
{
	PanitentApp* pPanitentApp;
	DockHostWindow* pDockHostTarget;
	FloatingDocumentSessionEntry* pEntry;
	BOOL abConsumed[16];
} FloatingDocumentRestoreContext;

typedef struct FloatingDocumentWorkspaceRecoveryPathContext
{
	int nFloatingIndex;
	int nWorkspaceIndex;
} FloatingDocumentWorkspaceRecoveryPathContext;

static BOOL FloatingDocumentPersist_BuildRecoveryPath(
	void* pUserData,
	int nIndex,
	WCHAR* pszBuffer,
	size_t cchBuffer)
{
	FloatingDocumentWorkspaceRecoveryPathContext* pContext = (FloatingDocumentWorkspaceRecoveryPathContext*)pUserData;
	if (!pContext || !pszBuffer || cchBuffer == 0)
	{
		return FALSE;
	}

	WCHAR szRelative[MAX_PATH] = L"";
	StringCchPrintfW(
		szRelative,
		ARRAYSIZE(szRelative),
		L"recovery_floatdoc_%02d_%02d_%02d.pdr",
		pContext->nFloatingIndex,
		pContext->nWorkspaceIndex,
		nIndex);
	PTSTR pszRecoveryPath = NULL;
	GetAppDataFilePath(szRelative, &pszRecoveryPath);
	if (!pszRecoveryPath)
	{
		return FALSE;
	}

	StringCchCopyW(pszBuffer, cchBuffer, pszRecoveryPath);
	free(pszRecoveryPath);
	return TRUE;
}

static void FloatingDocumentPersist_CollectWorkspaceSessionsRecursive(
	const DockModelNode* pLayoutNode,
	HWND* pWorkspaceHwnds,
	int nWorkspaceCount,
	int nFloatingIndex,
	FloatingDocumentSessionEntry* pEntry,
	int* pnWorkspaceIndex)
{
	if (!pLayoutNode || !pEntry || !pnWorkspaceIndex)
	{
		return;
	}

	if (pLayoutNode->nRole == DOCK_ROLE_WORKSPACE)
	{
		int idx = *pnWorkspaceIndex;
		if (idx >= 0 && idx < nWorkspaceCount && idx < ARRAYSIZE(pEntry->workspaces))
		{
			WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(pWorkspaceHwnds[idx]);
			if (pWorkspace)
			{
				FloatingDocumentWorkspaceSession* pWorkspaceSession = &pEntry->workspaces[pEntry->nWorkspaceCount];
				memset(pWorkspaceSession, 0, sizeof(*pWorkspaceSession));
				pWorkspaceSession->uWorkspaceNodeId = pLayoutNode->uNodeId;
				FloatingDocumentWorkspaceRecoveryPathContext pathContext = {
					nFloatingIndex,
					pEntry->nWorkspaceCount
				};
				DocumentSessionWorkspace_CaptureEntries(
					pWorkspace,
					pWorkspaceSession->entries,
					ARRAYSIZE(pWorkspaceSession->entries),
					&pWorkspaceSession->nFileCount,
					&pWorkspaceSession->nActiveEntry,
					FloatingDocumentPersist_BuildRecoveryPath,
					&pathContext);

				if (pWorkspaceSession->nFileCount > 0)
				{
					pEntry->nWorkspaceCount++;
				}
			}
		}

		(*pnWorkspaceIndex)++;
		return;
	}

	FloatingDocumentPersist_CollectWorkspaceSessionsRecursive(pLayoutNode->pChild1, pWorkspaceHwnds, nWorkspaceCount, nFloatingIndex, pEntry, pnWorkspaceIndex);
	FloatingDocumentPersist_CollectWorkspaceSessionsRecursive(pLayoutNode->pChild2, pWorkspaceHwnds, nWorkspaceCount, nFloatingIndex, pEntry, pnWorkspaceIndex);
}

static BOOL FloatingDocumentPersist_OnNodeAttached(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pNode,
	DockData* pDockData,
	Window* pWindow,
	void* pUserData)
{
	UNREFERENCED_PARAMETER(pPanitentApp);
	UNREFERENCED_PARAMETER(pDockHostWindow);
	UNREFERENCED_PARAMETER(pNode);
	if (!pDockData || !pWindow || !pUserData)
	{
		return FALSE;
	}

	if (pDockData->nRole != DOCK_ROLE_WORKSPACE)
	{
		return TRUE;
	}

	FloatingDocumentRestoreContext* pContext = (FloatingDocumentRestoreContext*)pUserData;
	int iWorkspaceIndex = -1;
	for (int i = 0; i < pContext->pEntry->nWorkspaceCount; ++i)
	{
		if (pContext->abConsumed[i])
		{
			continue;
		}
		if (pContext->pEntry->workspaces[i].uWorkspaceNodeId == pDockData->uModelNodeId ||
			pContext->pEntry->workspaces[i].uWorkspaceNodeId == 0)
		{
			iWorkspaceIndex = i;
			break;
		}
	}
	if (iWorkspaceIndex < 0)
	{
		return FALSE;
	}

	WorkspaceContainer* pWorkspaceContainer = (WorkspaceContainer*)pWindow;
	pContext->abConsumed[iWorkspaceIndex] = TRUE;
	FloatingDocumentWorkspaceSession* pWorkspaceSession = &pContext->pEntry->workspaces[iWorkspaceIndex];
	return DocumentSessionWorkspace_RestoreEntries(
		pWorkspaceContainer,
		pWorkspaceSession->entries,
		pWorkspaceSession->nFileCount,
		pWorkspaceSession->nActiveEntry);
}

static BOOL FloatingDocumentPersist_OnPinnedWindowCapture(
	HWND hWnd,
	FloatingWindowContainer* pFloatingWindowContainer,
	void* pUserData)
{
	FloatingDocumentPersistCollectContext* pContext = (FloatingDocumentPersistCollectContext*)pUserData;
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

	HWND hWndWorkspaces[64] = { 0 };
	int nWorkspaceCount = 0;
	FloatingDocumentSessionEntry* pEntry = &pContext->pModel->entries[pContext->pModel->nEntryCount++];
	memset(pEntry, 0, sizeof(*pEntry));
	if (!FloatingDocumentHost_CapturePinnedWindowState(
		hWnd,
		pFloatingWindowContainer,
		&pEntry->rcWindow,
		&pEntry->pLayoutModel,
		hWndWorkspaces,
		ARRAYSIZE(hWndWorkspaces),
		&nWorkspaceCount) ||
		nWorkspaceCount <= 0)
	{
		pContext->pModel->nEntryCount--;
		return TRUE;
	}

	int nWorkspaceIndex = 0;
	FloatingDocumentPersist_CollectWorkspaceSessionsRecursive(
		pEntry->pLayoutModel,
		hWndWorkspaces,
		nWorkspaceCount,
		pContext->pModel->nEntryCount - 1,
		pEntry,
		&nWorkspaceIndex);

	if (pEntry->nWorkspaceCount <= 0)
	{
		DockModel_Destroy(pEntry->pLayoutModel);
		pEntry->pLayoutModel = NULL;
		pContext->pModel->nEntryCount--;
	}

	return TRUE;
}

BOOL PanitentFloatingDocumentSession_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	UNREFERENCED_PARAMETER(pPanitentApp);
	UNREFERENCED_PARAMETER(pDockHostWindow);

	FloatingDocumentSessionModel* pModel = (FloatingDocumentSessionModel*)calloc(1, sizeof(FloatingDocumentSessionModel));
	if (!pModel)
	{
		return FALSE;
	}

	RecoveryStore_DeleteFilesByPattern(L"recovery_floatdoc_*.pdr", NULL);

	FloatingDocumentPersistCollectContext context = { pModel };
	PTSTR pszFilePath = NULL;
	FloatingDocumentHost_ForEachPinnedWindow(FloatingDocumentPersist_OnPinnedWindowCapture, &context);

	GetFloatingDocumentSessionFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		FloatingDocumentSessionModel_Destroy(pModel);
		free(pModel);
		return FALSE;
	}

	BOOL bResult = FloatingDocumentSessionModel_SaveToFile(pModel, pszFilePath);
	free(pszFilePath);
	FloatingDocumentSessionModel_Destroy(pModel);
	free(pModel);
	return bResult;
}

BOOL PanitentFloatingDocumentSession_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	FloatingDocumentSessionModel* pModel = (FloatingDocumentSessionModel*)calloc(1, sizeof(FloatingDocumentSessionModel));
	if (!pModel)
	{
		return FALSE;
	}

	PTSTR pszFilePath = NULL;
	if (!pPanitentApp || !pDockHostWindow)
	{
		free(pModel);
		return FALSE;
	}

	GetFloatingDocumentSessionFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		free(pModel);
		return FALSE;
	}

	PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
	BOOL bLoaded = FloatingDocumentSessionModel_LoadFromFileEx(pszFilePath, pModel, &loadStatus);
	if (!bLoaded && loadStatus == PERSIST_LOAD_INVALID_FORMAT)
	{
		PersistFile_QuarantineInvalid(pszFilePath);
	}
	free(pszFilePath);
	if (!bLoaded)
	{
		free(pModel);
		return FALSE;
	}

	BOOL bRestoredAny = FALSE;
	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		FloatingDocumentSessionEntry* pEntry = &pModel->entries[i];
		if (!pEntry->pLayoutModel || pEntry->nWorkspaceCount <= 0)
		{
			continue;
		}

		DockHostWindow* pFloatingDockHost = NULL;
		HWND hWndFloating = NULL;
		FloatingDocumentRestoreContext restoreContext = { 0 };
		restoreContext.pPanitentApp = pPanitentApp;
		restoreContext.pDockHostTarget = pDockHostWindow;
		restoreContext.pEntry = pEntry;
		BOOL bHasWorkspace = FALSE;
		if (!FloatingDocumentHost_RestorePinnedDockHost(
			pPanitentApp,
			pDockHostWindow,
			&pEntry->rcWindow,
			pEntry->pLayoutModel,
			NULL,
			NULL,
			FloatingDocumentPersist_OnNodeAttached,
			&restoreContext,
			&bHasWorkspace,
			&pFloatingDockHost,
			&hWndFloating))
		{
			continue;
		}

		bRestoredAny = TRUE;
	}

	FloatingDocumentSessionModel_Destroy(pModel);
	free(pModel);

	return bRestoredAny;
}

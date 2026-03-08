#include "precomp.h"

#include "floatingdocumentsessionpersist.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "document.h"
#include "documentrecovery.h"
#include "dockhost.h"
#include "dockhostrestore.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "floatingchildhost.h"
#include "floatingdocumentsessionmodel.h"
#include "floatingwindowcontainer.h"
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
	int nWorkspaceIndex;
} FloatingDocumentRestoreContext;

static BOOL FloatingDocumentPersist_IsClassName(HWND hWnd, PCWSTR pszClassName)
{
	WCHAR szClassName[64] = L"";
	if (!hWnd || !IsWindow(hWnd) || !pszClassName)
	{
		return FALSE;
	}

	GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
	return wcscmp(szClassName, pszClassName) == 0;
}

static DockModelNode* FloatingDocumentPersist_CaptureChildLayout(HWND hWndChild)
{
	FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
	if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_HOST)
	{
		DockHostWindow* pDockHostWindow = (DockHostWindow*)WindowMap_Get(hWndChild);
		return pDockHostWindow ? DockModel_CaptureHostLayout(pDockHostWindow) : NULL;
	}

	if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE)
	{
		DockModelNode* pRoot = (DockModelNode*)calloc(1, sizeof(DockModelNode));
		DockModelNode* pWorkspace = (DockModelNode*)calloc(1, sizeof(DockModelNode));
		if (!pRoot || !pWorkspace)
		{
			free(pRoot);
			free(pWorkspace);
			return NULL;
		}

		pRoot->nRole = DOCK_ROLE_ROOT;
		wcscpy_s(pRoot->szName, ARRAYSIZE(pRoot->szName), L"Root");
		pRoot->pChild1 = pWorkspace;

		pWorkspace->nRole = DOCK_ROLE_WORKSPACE;
		pWorkspace->nPaneKind = DOCK_PANE_DOCUMENT;
		wcscpy_s(pWorkspace->szName, ARRAYSIZE(pWorkspace->szName), L"WorkspaceContainer");
		return pRoot;
	}

	return NULL;
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
				pWorkspaceSession->nActiveEntry = -1;

				ViewportWindow* pActiveViewport = WorkspaceContainer_GetCurrentViewport(pWorkspace);
				for (int i = 0; i < WorkspaceContainer_GetViewportCount(pWorkspace) &&
					pWorkspaceSession->nFileCount < ARRAYSIZE(pWorkspaceSession->entries); ++i)
				{
					ViewportWindow* pViewportWindow = WorkspaceContainer_GetViewportAt(pWorkspace, i);
					Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
					if (!pDocument)
					{
						continue;
					}

					DocumentSessionEntry* pDocEntry = &pWorkspaceSession->entries[pWorkspaceSession->nFileCount];
					PCWSTR pszFilePath = Document_GetFilePath(pDocument);
					if (Document_IsFilePathSet(pDocument) && pszFilePath && pszFilePath[0] && !Document_IsDirty(pDocument))
					{
						pDocEntry->nKind = DOCSESSION_ENTRY_FILE;
						wcscpy_s(pDocEntry->szFilePath, ARRAYSIZE(pDocEntry->szFilePath), pszFilePath);
					}
					else {
						pDocEntry->nKind = DOCSESSION_ENTRY_RECOVERY;
						WCHAR szRelative[MAX_PATH] = L"";
						StringCchPrintfW(
							szRelative,
							ARRAYSIZE(szRelative),
							L"recovery_floatdoc_%02d_%02d_%02d.pdr",
							nFloatingIndex,
							pEntry->nWorkspaceCount,
							pWorkspaceSession->nFileCount);
						PTSTR pszRecoveryPath = NULL;
						GetAppDataFilePath(szRelative, &pszRecoveryPath);
						if (!pszRecoveryPath || !DocumentRecovery_Save(pDocument, pszRecoveryPath))
						{
							free(pszRecoveryPath);
							continue;
						}
						StringCchCopyW(pDocEntry->szFilePath, ARRAYSIZE(pDocEntry->szFilePath), pszRecoveryPath);
						free(pszRecoveryPath);
					}
					if (pViewportWindow == pActiveViewport)
					{
						pWorkspaceSession->nActiveEntry = pWorkspaceSession->nFileCount;
					}
					pWorkspaceSession->nFileCount++;
				}

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
	if (pContext->nWorkspaceIndex >= pContext->pEntry->nWorkspaceCount)
	{
		return FALSE;
	}

	WorkspaceContainer* pWorkspaceContainer = (WorkspaceContainer*)pWindow;
	FloatingDocumentWorkspaceSession* pWorkspaceSession = &pContext->pEntry->workspaces[pContext->nWorkspaceIndex++];
	for (int i = 0; i < pWorkspaceSession->nFileCount; ++i)
	{
		if (i == pWorkspaceSession->nActiveEntry)
		{
			continue;
		}
			DocumentSessionEntry* pDocEntry = &pWorkspaceSession->entries[i];
			if (pDocEntry->nKind == DOCSESSION_ENTRY_FILE)
			{
				Document_OpenFileInWorkspace(pDocEntry->szFilePath, pWorkspaceContainer);
			}
			else if (pDocEntry->nKind == DOCSESSION_ENTRY_RECOVERY)
			{
				DocumentRecovery_OpenInWorkspace(pDocEntry->szFilePath, pWorkspaceContainer);
			}
		}
		if (pWorkspaceSession->nActiveEntry >= 0 && pWorkspaceSession->nActiveEntry < pWorkspaceSession->nFileCount)
		{
			DocumentSessionEntry* pDocEntry = &pWorkspaceSession->entries[pWorkspaceSession->nActiveEntry];
			if (pDocEntry->nKind == DOCSESSION_ENTRY_FILE)
			{
				Document_OpenFileInWorkspace(pDocEntry->szFilePath, pWorkspaceContainer);
			}
			else if (pDocEntry->nKind == DOCSESSION_ENTRY_RECOVERY)
			{
				DocumentRecovery_OpenInWorkspace(pDocEntry->szFilePath, pWorkspaceContainer);
			}
		}

	return TRUE;
}

static BOOL CALLBACK FloatingDocumentPersist_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	FloatingDocumentPersistCollectContext* pContext = (FloatingDocumentPersistCollectContext*)lParam;
	DWORD processId = 0;
	if (!pContext || !pContext->pModel || !IsWindow(hWnd))
	{
		return TRUE;
	}

	GetWindowThreadProcessId(hWnd, &processId);
	if (processId != GetCurrentProcessId() ||
		!FloatingDocumentPersist_IsClassName(hWnd, L"__FloatingWindowContainer"))
	{
		return TRUE;
	}

	Window* pWindow = WindowMap_Get(hWnd);
	FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)pWindow;
	if (!pFloatingWindowContainer ||
		pFloatingWindowContainer->nDockPolicy != FLOAT_DOCK_POLICY_DOCUMENT ||
		pContext->pModel->nEntryCount >= ARRAYSIZE(pContext->pModel->entries))
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
	int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
		pFloatingWindowContainer->hWndChild,
		hWndWorkspaces,
		ARRAYSIZE(hWndWorkspaces));
	if (nWorkspaceCount <= 0)
	{
		return TRUE;
	}

	FloatingDocumentSessionEntry* pEntry = &pContext->pModel->entries[pContext->pModel->nEntryCount++];
	memset(pEntry, 0, sizeof(*pEntry));
	GetWindowRect(hWnd, &pEntry->rcWindow);
	pEntry->pLayoutModel = FloatingDocumentPersist_CaptureChildLayout(pFloatingWindowContainer->hWndChild);
	if (!pEntry->pLayoutModel)
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
	EnumWindows(FloatingDocumentPersist_EnumWindowsProc, (LPARAM)&context);

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
		DeleteFileW(pszFilePath);
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

		FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
		HWND hWndFloating = pFloatingWindowContainer ? Window_CreateWindow((Window*)pFloatingWindowContainer, NULL) : NULL;
		if (!pFloatingWindowContainer || !hWndFloating || !IsWindow(hWndFloating))
		{
			continue;
		}

		DockHostWindow* pFloatingDockHost = DockHostWindow_Create(pPanitentApp);
		HWND hWndFloatingDockHost = pFloatingDockHost ? Window_CreateWindow((Window*)pFloatingDockHost, NULL) : NULL;
		if (!pFloatingDockHost || !hWndFloatingDockHost || !IsWindow(hWndFloatingDockHost))
		{
			DestroyWindow(hWndFloating);
			continue;
		}

		FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
		FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_DOCUMENT);
		FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndFloatingDockHost);

		TreeNode* pRootNode = DockModelBuildTree(pEntry->pLayoutModel);
		if (!pRootNode || !pRootNode->data)
		{
			DestroyWindow(hWndFloating);
			continue;
		}

		RECT rcDockHost = { 0 };
		GetClientRect(hWndFloatingDockHost, &rcDockHost);
		((DockData*)pRootNode->data)->rc = rcDockHost;
		DockHostWindow_SetRoot(pFloatingDockHost, pRootNode);

		FloatingDocumentRestoreContext restoreContext = { 0 };
		restoreContext.pPanitentApp = pPanitentApp;
		restoreContext.pDockHostTarget = pDockHostWindow;
		restoreContext.pEntry = pEntry;
		restoreContext.nWorkspaceIndex = 0;
		BOOL bHasWorkspace = FALSE;
		if (!PanitentDockHostRestoreAttachKnownViewsEx(
			pPanitentApp,
			pFloatingDockHost,
			pRootNode,
			FloatingDocumentPersist_OnNodeAttached,
			&restoreContext,
			&bHasWorkspace))
		{
			DestroyWindow(hWndFloating);
			continue;
		}

		DockHostWindow_Rearrange(pFloatingDockHost);

		SetWindowPos(
			hWndFloating,
			HWND_TOP,
			pEntry->rcWindow.left,
			pEntry->rcWindow.top,
			max(1, pEntry->rcWindow.right - pEntry->rcWindow.left),
			max(1, pEntry->rcWindow.bottom - pEntry->rcWindow.top),
			SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
		bRestoredAny = TRUE;
	}

	FloatingDocumentSessionModel_Destroy(pModel);
	free(pModel);

	return bRestoredAny;
}

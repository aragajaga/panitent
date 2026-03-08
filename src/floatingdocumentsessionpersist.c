#include "precomp.h"

#include "floatingdocumentsessionpersist.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "document.h"
#include "dockhost.h"
#include "floatingchildhost.h"
#include "floatingdocumentsessionmodel.h"
#include "floatingwindowcontainer.h"
#include "panitentapp.h"
#include "shell/pathutil.h"
#include "viewport.h"
#include "workspacecontainer.h"

typedef struct FloatingDocumentPersistCollectContext
{
	FloatingDocumentSessionModel* pModel;
} FloatingDocumentPersistCollectContext;

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
	pEntry->nActiveEntry = -1;
	GetWindowRect(hWnd, &pEntry->rcWindow);

	for (int i = 0; i < nWorkspaceCount && pEntry->nFileCount < ARRAYSIZE(pEntry->szFilePaths); ++i)
	{
		WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndWorkspaces[i]);
		if (!pWorkspace)
		{
			continue;
		}

		ViewportWindow* pActiveViewport = WorkspaceContainer_GetCurrentViewport(pWorkspace);
		for (int j = 0; j < WorkspaceContainer_GetViewportCount(pWorkspace) && pEntry->nFileCount < ARRAYSIZE(pEntry->szFilePaths); ++j)
		{
			ViewportWindow* pViewportWindow = WorkspaceContainer_GetViewportAt(pWorkspace, j);
			Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
			PCWSTR pszFilePath = pDocument ? Document_GetFilePath(pDocument) : NULL;
			if (!pDocument || !Document_IsFilePathSet(pDocument) || !pszFilePath || !pszFilePath[0])
			{
				continue;
			}

			wcscpy_s(
				pEntry->szFilePaths[pEntry->nFileCount],
				ARRAYSIZE(pEntry->szFilePaths[pEntry->nFileCount]),
				pszFilePath);
			if (pViewportWindow == pActiveViewport)
			{
				pEntry->nActiveEntry = pEntry->nFileCount;
			}
			pEntry->nFileCount++;
		}
	}

	if (pEntry->nFileCount <= 0)
	{
		pContext->pModel->nEntryCount--;
	}

	return TRUE;
}

BOOL PanitentFloatingDocumentSession_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	UNREFERENCED_PARAMETER(pPanitentApp);
	UNREFERENCED_PARAMETER(pDockHostWindow);

	FloatingDocumentSessionModel model = { 0 };
	FloatingDocumentPersistCollectContext context = { &model };
	PTSTR pszFilePath = NULL;
	EnumWindows(FloatingDocumentPersist_EnumWindowsProc, (LPARAM)&context);

	GetFloatingDocumentSessionFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bResult = FloatingDocumentSessionModel_SaveToFile(&model, pszFilePath);
	free(pszFilePath);
	return bResult;
}

BOOL PanitentFloatingDocumentSession_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	FloatingDocumentSessionModel model = { 0 };
	PTSTR pszFilePath = NULL;
	if (!pPanitentApp || !pDockHostWindow)
	{
		return FALSE;
	}

	GetFloatingDocumentSessionFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bLoaded = FloatingDocumentSessionModel_LoadFromFile(pszFilePath, &model);
	free(pszFilePath);
	if (!bLoaded)
	{
		return FALSE;
	}

	BOOL bRestoredAny = FALSE;
	for (int i = 0; i < model.nEntryCount; ++i)
	{
		FloatingDocumentSessionEntry* pEntry = &model.entries[i];
		if (pEntry->nFileCount <= 0)
		{
			continue;
		}

		FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
		HWND hWndFloating = pFloatingWindowContainer ? Window_CreateWindow((Window*)pFloatingWindowContainer, NULL) : NULL;
		if (!pFloatingWindowContainer || !hWndFloating || !IsWindow(hWndFloating))
		{
			continue;
		}

		WorkspaceContainer* pFloatingWorkspace = WorkspaceContainer_Create();
		HWND hWndFloatingWorkspace = pFloatingWorkspace ? Window_CreateWindow((Window*)pFloatingWorkspace, NULL) : NULL;
		if (!pFloatingWorkspace || !hWndFloatingWorkspace || !IsWindow(hWndFloatingWorkspace))
		{
			DestroyWindow(hWndFloating);
			continue;
		}

		FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
		FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_DOCUMENT);
		FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndFloatingWorkspace);

		for (int j = 0; j < pEntry->nFileCount; ++j)
		{
			if (j == pEntry->nActiveEntry)
			{
				continue;
			}

			Document_OpenFileInWorkspace(pEntry->szFilePaths[j], pFloatingWorkspace);
		}

		if (pEntry->nActiveEntry >= 0 && pEntry->nActiveEntry < pEntry->nFileCount)
		{
			Document_OpenFileInWorkspace(pEntry->szFilePaths[pEntry->nActiveEntry], pFloatingWorkspace);
		}

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

	return bRestoredAny;
}

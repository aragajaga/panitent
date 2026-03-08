#include "precomp.h"

#include "dockfloatingpersist.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "dockgroup.h"
#include "dockviewfactory.h"
#include "floatingwindowcontainer.h"
#include "floatingchildhost.h"
#include "shell/pathutil.h"

typedef struct DockFloatingPersistCollectContext
{
	DockFloatingLayoutFileModel* pModel;
} DockFloatingPersistCollectContext;

static BOOL DockFloatingPersist_IsClassName(HWND hWnd, PCWSTR pszClassName)
{
	WCHAR szClassName[64] = L"";
	if (!hWnd || !IsWindow(hWnd) || !pszClassName)
	{
		return FALSE;
	}

	GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
	return wcscmp(szClassName, pszClassName) == 0;
}

static BOOL DockFloatingPersist_MainHostHasView(TreeNode* pNode, PanitentDockViewId nViewId)
{
	if (!pNode || nViewId == PNT_DOCK_VIEW_NONE)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData && PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName) == nViewId)
	{
		return TRUE;
	}

	return DockFloatingPersist_MainHostHasView(pNode->node1, nViewId) ||
		DockFloatingPersist_MainHostHasView(pNode->node2, nViewId);
}

static BOOL CALLBACK DockFloatingPersist_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	DockFloatingPersistCollectContext* pContext = (DockFloatingPersistCollectContext*)lParam;
	DWORD processId = 0;
	if (!pContext || !pContext->pModel || !IsWindow(hWnd))
	{
		return TRUE;
	}

	GetWindowThreadProcessId(hWnd, &processId);
	if (processId != GetCurrentProcessId() ||
		!DockFloatingPersist_IsClassName(hWnd, L"__FloatingWindowContainer"))
	{
		return TRUE;
	}

	Window* pWindow = WindowMap_Get(hWnd);
	FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)pWindow;
	if (!pFloatingWindowContainer ||
		pFloatingWindowContainer->nDockPolicy != FLOAT_DOCK_POLICY_PANEL ||
		pContext->pModel->nEntries >= ARRAYSIZE(pContext->pModel->entries))
	{
		return TRUE;
	}

	if (FloatingChildHost_GetKind(pFloatingWindowContainer->hWndChild) != FLOAT_DOCK_CHILD_TOOL_PANEL)
	{
		return TRUE;
	}

	WCHAR szTitle[128] = L"";
	WCHAR szClassName[64] = L"";
	GetWindowTextW(pFloatingWindowContainer->hWndChild, szTitle, ARRAYSIZE(szTitle));
	GetClassNameW(pFloatingWindowContainer->hWndChild, szClassName, ARRAYSIZE(szClassName));

	PanitentDockViewId nViewId = PanitentDockViewCatalog_FindForWindow(szClassName, szTitle);
	if (nViewId == PNT_DOCK_VIEW_NONE)
	{
		return TRUE;
	}

	DockFloatingLayoutEntry* pEntry = &pContext->pModel->entries[pContext->pModel->nEntries++];
	pEntry->nViewId = nViewId;
	GetWindowRect(hWnd, &pEntry->rcWindow);
	pEntry->iDockSizeHint = pFloatingWindowContainer->iDockSizeHint;
	return TRUE;
}

BOOL PanitentDockFloating_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	UNREFERENCED_PARAMETER(pPanitentApp);
	UNREFERENCED_PARAMETER(pDockHostWindow);

	DockFloatingLayoutFileModel model = { 0 };
	DockFloatingPersistCollectContext context = { &model };
	PTSTR pszFilePath = NULL;
	EnumWindows(DockFloatingPersist_EnumWindowsProc, (LPARAM)&context);

	GetDockFloatingFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bResult = DockFloatingLayout_SaveToFile(&model, pszFilePath);
	free(pszFilePath);
	return bResult;
}

BOOL PanitentDockFloating_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	DockFloatingLayoutFileModel model = { 0 };
	PTSTR pszFilePath = NULL;
	GetDockFloatingFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bLoaded = DockFloatingLayout_LoadFromFile(pszFilePath, &model);
	free(pszFilePath);
	if (!bLoaded)
	{
		return FALSE;
	}

	BOOL bRestoredAny = FALSE;
	for (int i = 0; i < model.nEntries; ++i)
	{
		DockFloatingLayoutEntry* pEntry = &model.entries[i];
		if (DockFloatingPersist_MainHostHasView(DockHostWindow_GetRoot(pDockHostWindow), pEntry->nViewId))
		{
			continue;
		}

		FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
		HWND hWndFloating = pFloatingWindowContainer ? Window_CreateWindow((Window*)pFloatingWindowContainer, NULL) : NULL;
		if (!pFloatingWindowContainer || !hWndFloating || !IsWindow(hWndFloating))
		{
			continue;
		}

		Window* pChildWindow = PanitentDockViewFactory_CreateWindow(pPanitentApp, pEntry->nViewId);
		if (!pChildWindow || !Window_CreateWindow(pChildWindow, NULL))
		{
			DestroyWindow(hWndFloating);
			continue;
		}

		FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
		FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_PANEL);
		FloatingWindowContainer_PinWindow(pFloatingWindowContainer, Window_GetHWND(pChildWindow));
		pFloatingWindowContainer->iDockSizeHint = pEntry->iDockSizeHint;
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

#include "precomp.h"

#include "floatingdocumentlayoutpersist.h"

#include "dockhost.h"
#include "dockhostrestore.h"
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

static BOOL FloatingDocumentLayout_IsClassName(HWND hWnd, PCWSTR pszClassName)
{
	WCHAR szClassName[64] = L"";
	if (!hWnd || !IsWindow(hWnd) || !pszClassName)
	{
		return FALSE;
	}

	GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
	return wcscmp(szClassName, pszClassName) == 0;
}

static DockModelNode* FloatingDocumentLayout_CaptureChildLayout(HWND hWndChild)
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

static BOOL CALLBACK FloatingDocumentLayout_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	FloatingDocumentLayoutCaptureContext* pContext = (FloatingDocumentLayoutCaptureContext*)lParam;
	DWORD processId = 0;
	if (!pContext || !pContext->pModel || !IsWindow(hWnd))
	{
		return TRUE;
	}

	GetWindowThreadProcessId(hWnd, &processId);
	if (processId != GetCurrentProcessId() ||
		!FloatingDocumentLayout_IsClassName(hWnd, L"__FloatingWindowContainer"))
	{
		return TRUE;
	}

	FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)WindowMap_Get(hWnd);
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

	FloatingDocumentLayoutEntry* pEntry = &pContext->pModel->entries[pContext->pModel->nEntryCount];
	memset(pEntry, 0, sizeof(*pEntry));
	GetWindowRect(hWnd, &pEntry->rcWindow);
	pEntry->pLayoutModel = FloatingDocumentLayout_CaptureChildLayout(pFloatingWindowContainer->hWndChild);
	if (!pEntry->pLayoutModel)
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
	EnumWindows(FloatingDocumentLayout_EnumWindowsProc, (LPARAM)&context);
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

static BOOL CALLBACK FloatingDocumentLayout_CollectLiveWindowsProc(HWND hWnd, LPARAM lParam)
{
	FloatingDocumentLayoutApplyContext* pContext = (FloatingDocumentLayoutApplyContext*)lParam;
	DWORD processId = 0;
	if (!pContext || !IsWindow(hWnd))
	{
		return TRUE;
	}

	GetWindowThreadProcessId(hWnd, &processId);
	if (processId != GetCurrentProcessId() ||
		!FloatingDocumentLayout_IsClassName(hWnd, L"__FloatingWindowContainer"))
	{
		return TRUE;
	}

	FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)WindowMap_Get(hWnd);
	if (!pFloatingWindowContainer || pFloatingWindowContainer->nDockPolicy != FLOAT_DOCK_POLICY_DOCUMENT)
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
	EnumWindows(FloatingDocumentLayout_CollectLiveWindowsProc, (LPARAM)&context);

	BOOL bRestoredAny = FALSE;
	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		const FloatingDocumentLayoutEntry* pEntry = &pModel->entries[i];
		if (!pEntry->pLayoutModel)
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

		BOOL bHasWorkspace = FALSE;
		if (!PanitentDockHostRestoreAttachKnownViewsEx(
			pPanitentApp,
			pFloatingDockHost,
			pRootNode,
			FloatingDocumentLayout_ResolveWorkspace,
			&context,
			NULL,
			NULL,
			&bHasWorkspace))
		{
			DockHostWindow_DestroyNodeTree(pRootNode, NULL, 0);
			DestroyWindow(hWndFloating);
			continue;
		}

		DockHostWindow_SetRoot(pFloatingDockHost, pRootNode);
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

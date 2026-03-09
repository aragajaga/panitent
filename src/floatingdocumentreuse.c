#include "precomp.h"

#include "floatingdocumenthost.h"

#include "dockmodel.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"
#include "workspacecontainer.h"

typedef struct FloatingDocumentHostEnumContext
{
    FnFloatingDocumentHostWindowCallback pfnCallback;
    void* pUserData;
    BOOL bResult;
} FloatingDocumentHostEnumContext;

static BOOL CALLBACK FloatingDocumentHost_EnumPinnedWindowsProc(HWND hWnd, LPARAM lParam)
{
    FloatingDocumentHostEnumContext* pContext = (FloatingDocumentHostEnumContext*)lParam;
    FloatingWindowContainer* pFloatingWindowContainer = NULL;
    if (!pContext)
    {
        return TRUE;
    }

    if (!FloatingDocumentHost_IsPinnedFloatingWindow(hWnd, &pFloatingWindowContainer))
    {
        return TRUE;
    }

    if (!pContext->pfnCallback)
    {
        return TRUE;
    }

    if (!pContext->pfnCallback(hWnd, pFloatingWindowContainer, pContext->pUserData))
    {
        pContext->bResult = FALSE;
        return FALSE;
    }

    return TRUE;
}

BOOL FloatingDocumentHost_IsPinnedFloatingWindow(HWND hWnd, FloatingWindowContainer** ppFloatingWindowContainer)
{
    WCHAR szClassName[64] = L"";
    DWORD processId = 0;
    FloatingWindowContainer* pFloatingWindowContainer = NULL;

    if (ppFloatingWindowContainer)
    {
        *ppFloatingWindowContainer = NULL;
    }

    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    GetWindowThreadProcessId(hWnd, &processId);
    if (processId != GetCurrentProcessId())
    {
        return FALSE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    if (wcscmp(szClassName, L"__FloatingWindowContainer") != 0)
    {
        return FALSE;
    }

    pFloatingWindowContainer = (FloatingWindowContainer*)WindowMap_Get(hWnd);
    if (!pFloatingWindowContainer || pFloatingWindowContainer->nDockPolicy != FLOAT_DOCK_POLICY_DOCUMENT)
    {
        return FALSE;
    }

    if (ppFloatingWindowContainer)
    {
        *ppFloatingWindowContainer = pFloatingWindowContainer;
    }
    return TRUE;
}

BOOL FloatingDocumentHost_ForEachPinnedWindow(FnFloatingDocumentHostWindowCallback pfnCallback, void* pUserData)
{
    FloatingDocumentHostEnumContext context = { 0 };
    context.pfnCallback = pfnCallback;
    context.pUserData = pUserData;
    context.bResult = TRUE;
    EnumWindows(FloatingDocumentHost_EnumPinnedWindowsProc, (LPARAM)&context);
    return context.bResult;
}

static BOOL FloatingDocumentHost_OnPinnedWindowCollectLive(
    HWND hWnd,
    FloatingWindowContainer* pFloatingWindowContainer,
    void* pUserData)
{
    FloatingDocumentWorkspaceReuseContext* pContext = (FloatingDocumentWorkspaceReuseContext*)pUserData;
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
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        hWndChild,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    if (nWorkspaceCount > 0)
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

BOOL FloatingDocumentHost_CollectLiveWorkspaces(FloatingDocumentWorkspaceReuseContext* pContext)
{
    if (!pContext)
    {
        return FALSE;
    }

    memset(pContext, 0, sizeof(*pContext));
    return FloatingDocumentHost_ForEachPinnedWindow(FloatingDocumentHost_OnPinnedWindowCollectLive, pContext);
}

BOOL FloatingDocumentHost_PrepareWorkspaceReuse(
    FloatingDocumentWorkspaceReuseContext* pContext,
    BOOL bClearViewports)
{
    if (!FloatingDocumentHost_CollectLiveWorkspaces(pContext))
    {
        return FALSE;
    }

    if (!bClearViewports)
    {
        return TRUE;
    }

    for (int i = 0; i < pContext->nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(pContext->hWorkspaceHwnds[i]);
        if (pWorkspace)
        {
            WorkspaceContainer_ClearAllViewports(pWorkspace);
        }
    }

    return TRUE;
}

Window* FloatingDocumentHost_ResolveReusedWorkspace(
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

    FloatingDocumentWorkspaceReuseContext* pContext = (FloatingDocumentWorkspaceReuseContext*)pUserData;
    if (nViewId != PNT_DOCK_VIEW_WORKSPACE || !pContext)
    {
        return NULL;
    }

    if (pContext->iNextWorkspace < pContext->nWorkspaceCount)
    {
        HWND hWndWorkspace = pContext->hWorkspaceHwnds[pContext->iNextWorkspace++];
        return (Window*)WindowMap_Get(hWndWorkspace);
    }

    return (Window*)WorkspaceContainer_Create();
}

void FloatingDocumentHost_DisposeUnusedReusedWorkspaces(
    PanitentApp* pPanitentApp,
    FloatingDocumentWorkspaceReuseContext* pContext)
{
    if (!pContext || pContext->iNextWorkspace >= pContext->nWorkspaceCount)
    {
        return;
    }

    WorkspaceContainer* pMainWorkspace = PanitentApp_GetWorkspaceContainer(pPanitentApp);
    for (int i = pContext->iNextWorkspace; i < pContext->nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(pContext->hWorkspaceHwnds[i]);
        if (pWorkspace && pMainWorkspace)
        {
            WorkspaceContainer_MoveAllViewportsTo(pWorkspace, pMainWorkspace);
        }
        if (pContext->hWorkspaceHwnds[i] && IsWindow(pContext->hWorkspaceHwnds[i]))
        {
            DestroyWindow(pContext->hWorkspaceHwnds[i]);
        }
    }
}

BOOL FloatingDocumentHost_CapturePinnedWindowState(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    RECT* pWindowRect,
    DockModelNode** ppLayoutModel,
    HWND* pWorkspaceHwnds,
    int cWorkspaceHwnds,
    int* pnWorkspaceCount)
{
    if (ppLayoutModel)
    {
        *ppLayoutModel = NULL;
    }
    if (pnWorkspaceCount)
    {
        *pnWorkspaceCount = 0;
    }

    if (!hWndFloating || !IsWindow(hWndFloating) || !pFloatingWindowContainer)
    {
        return FALSE;
    }

    HWND hWndChild = pFloatingWindowContainer->hWndChild;
    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FALSE;
    }

    FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
    if (nChildKind != FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE &&
        nChildKind != FLOAT_DOCK_CHILD_DOCUMENT_HOST)
    {
        return FALSE;
    }

    if (pWindowRect)
    {
        GetWindowRect(hWndFloating, pWindowRect);
    }

    if (ppLayoutModel)
    {
        *ppLayoutModel = FloatingDocumentHost_CaptureChildLayout(hWndChild);
        if (!*ppLayoutModel)
        {
            return FALSE;
        }
    }

    if (pWorkspaceHwnds && cWorkspaceHwnds > 0)
    {
        int nCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(hWndChild, pWorkspaceHwnds, cWorkspaceHwnds);
        if (pnWorkspaceCount)
        {
            *pnWorkspaceCount = nCount;
        }
    }
    else if (pnWorkspaceCount)
    {
        *pnWorkspaceCount = 0;
    }

    return TRUE;
}

DockModelNode* FloatingDocumentHost_CaptureChildLayout(HWND hWndChild)
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

#include "precomp.h"

#include "floatingdocumenthost.h"

#include "dockmodel.h"
#include "dockhostrestore.h"
#include "dockmodelbuild.h"
#include "floatingchildhost.h"
#include "panitentapp.h"
#include "floatingwindowcontainer.h"
#include "win32/window.h"
#include "win32/windowmap.h"
#include "win32/util.h"
#include "workspacecontainer.h"

static FnFloatingDocumentHostCreatePinnedWindowHook g_pCreatePinnedWindowTestHook = NULL;

typedef struct FloatingDocumentHostEnumContext
{
    FnFloatingDocumentHostWindowCallback pfnCallback;
    void* pUserData;
    BOOL bResult;
} FloatingDocumentHostEnumContext;

void FloatingDocumentHost_SetCreatePinnedWindowTestHook(FnFloatingDocumentHostCreatePinnedWindowHook pfnHook)
{
    g_pCreatePinnedWindowTestHook = pfnHook;
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

BOOL FloatingDocumentHost_CreatePinnedWindow(
    DockHostWindow* pDockHostTarget,
    HWND hWndChild,
    const RECT* pWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut)
{
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }

    if (g_pCreatePinnedWindowTestHook)
    {
        return g_pCreatePinnedWindowTestHook(
            pDockHostTarget,
            hWndChild,
            pWindowRect,
            bStartMove,
            ptMoveScreen,
            phWndFloatingOut);
    }

    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FALSE;
    }

    FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
    HWND hWndFloating = pFloatingWindowContainer ? Window_CreateWindow((Window*)pFloatingWindowContainer, NULL) : NULL;
    if (!pFloatingWindowContainer || !hWndFloating || !IsWindow(hWndFloating))
    {
        free(pFloatingWindowContainer);
        return FALSE;
    }

    FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostTarget);
    FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_DOCUMENT);
    FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndChild);

    RECT rcWindow = { 0 };
    if (pWindowRect)
    {
        rcWindow = *pWindowRect;
    }
    else {
        GetWindowRect(hWndFloating, &rcWindow);
    }

    int width = max(1, Win32_Rect_GetWidth(&rcWindow));
    int height = max(1, Win32_Rect_GetHeight(&rcWindow));
    SetWindowPos(
        hWndFloating,
        HWND_TOP,
        rcWindow.left,
        rcWindow.top,
        width,
        height,
        SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);

    if (bStartMove)
    {
        SetForegroundWindow(hWndFloating);
        SendMessage(hWndFloating, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(ptMoveScreen.x, ptMoveScreen.y));
    }

    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

BOOL FloatingDocumentHost_CreatePinnedDockHost(
    DockHostWindow* pDockHostTarget,
    PanitentApp* pPanitentApp,
    const RECT* pWindowRect,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndDockHostOut,
    HWND* phWndFloatingOut)
{
    if (ppFloatingDockHostOut)
    {
        *ppFloatingDockHostOut = NULL;
    }
    if (phWndDockHostOut)
    {
        *phWndDockHostOut = NULL;
    }
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }

    DockHostWindow* pFloatingDockHost = DockHostWindow_Create(pPanitentApp);
    HWND hWndFloatingDockHost = pFloatingDockHost ? Window_CreateWindow((Window*)pFloatingDockHost, NULL) : NULL;
    if (!pFloatingDockHost || !hWndFloatingDockHost || !IsWindow(hWndFloatingDockHost))
    {
        return FALSE;
    }

    HWND hWndFloating = NULL;
    if (!FloatingDocumentHost_CreatePinnedWindow(
        pDockHostTarget,
        hWndFloatingDockHost,
        pWindowRect,
        FALSE,
        (POINT){ 0, 0 },
        &hWndFloating))
    {
        DestroyWindow(hWndFloatingDockHost);
        return FALSE;
    }

    if (ppFloatingDockHostOut)
    {
        *ppFloatingDockHostOut = pFloatingDockHost;
    }
    if (phWndDockHostOut)
    {
        *phWndDockHostOut = hWndFloatingDockHost;
    }
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

BOOL FloatingDocumentHost_RestorePinnedDockHost(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostTarget,
    const RECT* pWindowRect,
    const DockModelNode* pLayoutModel,
    FnDockHostRestoreResolveView pfnResolveView,
    void* pResolveViewUserData,
    FnDockHostRestoreNodeAttached pfnNodeAttached,
    void* pNodeAttachedUserData,
    BOOL* pbHasWorkspace,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndFloatingOut)
{
    if (pbHasWorkspace)
    {
        *pbHasWorkspace = FALSE;
    }
    if (ppFloatingDockHostOut)
    {
        *ppFloatingDockHostOut = NULL;
    }
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }

    if (!pPanitentApp || !pLayoutModel)
    {
        return FALSE;
    }

    DockHostWindow* pFloatingDockHost = NULL;
    HWND hWndFloatingDockHost = NULL;
    HWND hWndFloating = NULL;
    if (!FloatingDocumentHost_CreatePinnedDockHost(
        pDockHostTarget,
        pPanitentApp,
        pWindowRect,
        &pFloatingDockHost,
        &hWndFloatingDockHost,
        &hWndFloating))
    {
        return FALSE;
    }

    TreeNode* pRootNode = DockModelBuildTree(pLayoutModel);
    if (!pRootNode || !pRootNode->data)
    {
        DestroyWindow(hWndFloating);
        return FALSE;
    }

    RECT rcDockHost = { 0 };
    GetClientRect(hWndFloatingDockHost, &rcDockHost);
    ((DockData*)pRootNode->data)->rc = rcDockHost;

    BOOL bHasWorkspace = FALSE;
    if (!PanitentDockHostRestoreAttachKnownViewsEx(
        pPanitentApp,
        pFloatingDockHost,
        pRootNode,
        pfnResolveView,
        pResolveViewUserData,
        pfnNodeAttached,
        pNodeAttachedUserData,
        &bHasWorkspace))
    {
        DockHostWindow_DestroyNodeTree(pRootNode, NULL, 0);
        DestroyWindow(hWndFloating);
        return FALSE;
    }

    DockHostWindow_SetRoot(pFloatingDockHost, pRootNode);
    DockHostWindow_Rearrange(pFloatingDockHost);

    if (pbHasWorkspace)
    {
        *pbHasWorkspace = bHasWorkspace;
    }
    if (ppFloatingDockHostOut)
    {
        *ppFloatingDockHostOut = pFloatingDockHost;
    }
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

BOOL FloatingDocumentHost_RestorePinnedDockHostWithReuse(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostTarget,
    const RECT* pWindowRect,
    const DockModelNode* pLayoutModel,
    FloatingDocumentWorkspaceReuseContext* pReuse,
    FnDockHostRestoreNodeAttached pfnNodeAttached,
    void* pNodeAttachedUserData,
    BOOL* pbHasWorkspace,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndFloatingOut)
{
    return FloatingDocumentHost_RestorePinnedDockHost(
        pPanitentApp,
        pDockHostTarget,
        pWindowRect,
        pLayoutModel,
        FloatingDocumentHost_ResolveReusedWorkspace,
        pReuse,
        pfnNodeAttached,
        pNodeAttachedUserData,
        pbHasWorkspace,
        ppFloatingDockHostOut,
        phWndFloatingOut);
}

#include "precomp.h"

#include "floatingdocumenthost.h"

#include "dockhostrestore.h"
#include "dockmodelbuild.h"
#include "panitentapp.h"
#include "floatingwindowcontainer.h"
#include "win32/window.h"
#include "win32/util.h"

static FnFloatingDocumentHostCreatePinnedWindowHook g_pCreatePinnedWindowTestHook = NULL;

void FloatingDocumentHost_SetCreatePinnedWindowTestHook(FnFloatingDocumentHostCreatePinnedWindowHook pfnHook)
{
    g_pCreatePinnedWindowTestHook = pfnHook;
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

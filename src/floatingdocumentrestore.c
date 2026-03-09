#include "precomp.h"

#include "floatingdocumenthost.h"

#include "dockhostrestore.h"
#include "dockmodelbuild.h"

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

#include "precomp.h"

#include "floatingtoolrestore.h"

#include "dockhostrestore.h"
#include "dockmodelbuild.h"
#include "dockviewfactory.h"
#include "floatingwindowcontainer.h"

BOOL FloatingToolHost_RestoreEntry(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    const DockFloatingLayoutEntry* pEntry,
    HWND* phWndFloatingOut)
{
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }

    if (!pPanitentApp || !pDockHostWindow || !pEntry)
    {
        return FALSE;
    }

    FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
    HWND hWndFloating = pFloatingWindowContainer ? Window_CreateWindow((Window*)pFloatingWindowContainer, NULL) : NULL;
    if (!pFloatingWindowContainer || !hWndFloating || !IsWindow(hWndFloating))
    {
        return FALSE;
    }

    FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
    FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_PANEL);
    pFloatingWindowContainer->iDockSizeHint = pEntry->iDockSizeHint;

    if (pEntry->nChildKind == FLOAT_DOCK_CHILD_TOOL_PANEL)
    {
        Window* pChildWindow = PanitentDockViewFactory_CreateWindow(pPanitentApp, pEntry->nViewId);
        if (!pChildWindow || !Window_CreateWindow(pChildWindow, NULL))
        {
            DestroyWindow(hWndFloating);
            return FALSE;
        }

        FloatingWindowContainer_PinWindow(pFloatingWindowContainer, Window_GetHWND(pChildWindow));
    }
    else if (pEntry->nChildKind == FLOAT_DOCK_CHILD_TOOL_HOST && pEntry->pLayoutModel)
    {
        DockHostWindow* pFloatingDockHost = DockHostWindow_Create(pPanitentApp);
        HWND hWndFloatingDockHost = pFloatingDockHost ? Window_CreateWindow((Window*)pFloatingDockHost, NULL) : NULL;
        if (!pFloatingDockHost || !hWndFloatingDockHost || !IsWindow(hWndFloatingDockHost))
        {
            DestroyWindow(hWndFloating);
            return FALSE;
        }

        FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndFloatingDockHost);
        TreeNode* pRootNode = DockModelBuildTree(pEntry->pLayoutModel);
        if (!pRootNode || !pRootNode->data)
        {
            DestroyWindow(hWndFloating);
            return FALSE;
        }

        RECT rcDockHost = { 0 };
        GetClientRect(hWndFloatingDockHost, &rcDockHost);
        ((DockData*)pRootNode->data)->rc = rcDockHost;
        DockHostWindow_SetRoot(pFloatingDockHost, pRootNode);
        BOOL bHasWorkspace = FALSE;
        if (!PanitentDockHostRestoreAttachKnownViews(pPanitentApp, pFloatingDockHost, pRootNode, &bHasWorkspace))
        {
            DestroyWindow(hWndFloating);
            return FALSE;
        }

        DockHostWindow_Rearrange(pFloatingDockHost);
    }
    else {
        DestroyWindow(hWndFloating);
        return FALSE;
    }

    SetWindowPos(
        hWndFloating,
        HWND_TOP,
        pEntry->rcWindow.left,
        pEntry->rcWindow.top,
        max(1, pEntry->rcWindow.right - pEntry->rcWindow.left),
        max(1, pEntry->rcWindow.bottom - pEntry->rcWindow.top),
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED);

    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

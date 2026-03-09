#include "precomp.h"

#include "documentdocktransition.h"

#include "dockhost.h"
#include "floatingchildhost.h"
#include "workspacecontainer.h"
#include "win32/util.h"

BOOL DocumentDockTransition_DockSourceToWorkspace(
    HWND hWndSourceRoot,
    HWND* phWndSourceChild,
    WorkspaceContainer* pWorkspaceTarget,
    int nDockSide,
    int iDockSizeHint)
{
    if (!phWndSourceChild || !*phWndSourceChild || !IsWindow(*phWndSourceChild) || !pWorkspaceTarget)
    {
        return FALSE;
    }

    if (nDockSide == DKS_CENTER)
    {
        return FloatingChildHost_MoveDocumentsToWorkspace(*phWndSourceChild, pWorkspaceTarget);
    }

    if (nDockSide != DKS_LEFT &&
        nDockSide != DKS_RIGHT &&
        nDockSide != DKS_TOP &&
        nDockSide != DKS_BOTTOM)
    {
        return FALSE;
    }

    if (!hWndSourceRoot || !IsWindow(hWndSourceRoot))
    {
        return FALSE;
    }

    if (!FloatingChildHost_EnsureWorkspaceChildForSideDock(hWndSourceRoot, phWndSourceChild))
    {
        return FALSE;
    }

    HWND hWndTargetWorkspace = Window_GetHWND((Window*)pWorkspaceTarget);
    if (!hWndTargetWorkspace || !IsWindow(hWndTargetWorkspace))
    {
        return FALSE;
    }

    DockHostWindow* pTargetDockHostWindow = FloatingChildHost_EnsureTargetWorkspaceDockHost(hWndTargetWorkspace);
    if (!pTargetDockHostWindow)
    {
        return FALSE;
    }

    RECT rcSourceWindow = { 0 };
    GetWindowRect(hWndSourceRoot, &rcSourceWindow);
    int iDockSize = iDockSizeHint;
    if (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT)
    {
        iDockSize = max(iDockSize, Win32_Rect_GetWidth(&rcSourceWindow));
    }
    else {
        iDockSize = max(iDockSize, Win32_Rect_GetHeight(&rcSourceWindow));
    }
    iDockSize = max(iDockSize, 220);

    DockTargetHit dockTarget = { 0 };
    dockTarget.nDockSide = nDockSide;
    dockTarget.bLocalTarget = TRUE;
    dockTarget.hWndAnchor = hWndTargetWorkspace;

    HWND hWndChild = *phWndSourceChild;
    *phWndSourceChild = NULL;
    if (!DockHostWindow_DockHWNDToTarget(pTargetDockHostWindow, hWndChild, &dockTarget, iDockSize))
    {
        *phWndSourceChild = hWndChild;
        return FALSE;
    }

    return TRUE;
}

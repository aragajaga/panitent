#include "precomp.h"

#include "floatingdocumentdock.h"

#include "docklayout.h"
#include "documentdocktransition.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "panitentapp.h"
#include "workspacecontainer.h"
#include "workspacedockpolicy.h"
#include "win32/window.h"
#include "win32/util.h"

#define FLOATING_DOCUMENT_DOCK_GUIDE_SIZE 24
#define FLOATING_DOCUMENT_DOCK_GUIDE_GAP 8

static void FloatingDocumentDock_GetClusterGuideRects(
    const RECT* pBounds,
    int width,
    int height,
    RECT* pRectCenter,
    RECT* pRectTop,
    RECT* pRectLeft,
    RECT* pRectRight,
    RECT* pRectBottom)
{
    if (!pBounds)
    {
        return;
    }

    int guideSize = FLOATING_DOCUMENT_DOCK_GUIDE_SIZE;
    int guideStep = guideSize + FLOATING_DOCUMENT_DOCK_GUIDE_GAP;
    int cx = (pBounds->left + pBounds->right) / 2;
    int cy = (pBounds->top + pBounds->bottom) / 2;

    cx = max(guideStep, min(cx, width - guideStep));
    cy = max(guideStep, min(cy, height - guideStep));

    if (pRectCenter)
    {
        SetRect(pRectCenter, cx - guideSize / 2, cy - guideSize / 2, cx + guideSize / 2, cy + guideSize / 2);
    }
    if (pRectTop)
    {
        SetRect(pRectTop, cx - guideSize / 2, cy - guideStep - guideSize / 2, cx + guideSize / 2, cy - guideStep + guideSize / 2);
    }
    if (pRectLeft)
    {
        SetRect(pRectLeft, cx - guideStep - guideSize / 2, cy - guideSize / 2, cx - guideStep + guideSize / 2, cy + guideSize / 2);
    }
    if (pRectRight)
    {
        SetRect(pRectRight, cx + guideStep - guideSize / 2, cy - guideSize / 2, cx + guideStep + guideSize / 2, cy + guideSize / 2);
    }
    if (pRectBottom)
    {
        SetRect(pRectBottom, cx - guideSize / 2, cy + guideStep - guideSize / 2, cx + guideSize / 2, cy + guideStep + guideSize / 2);
    }
}

static BOOL FloatingDocumentDock_IsClassName(HWND hWnd, PCWSTR pszClassName)
{
    WCHAR szClassName[64] = L"";
    if (!hWnd || !IsWindow(hWnd) || !pszClassName)
    {
        return FALSE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return wcscmp(szClassName, pszClassName) == 0;
}

static BOOL FloatingDocumentDock_IsWorkspaceSplitDockSupported(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndWorkspace)
{
    UNREFERENCED_PARAMETER(pFloatingWindowContainer);

    if (!hWndWorkspace || !IsWindow(hWndWorkspace))
    {
        return FALSE;
    }

    HWND hWndParent = GetParent(hWndWorkspace);
    if (!hWndParent || !IsWindow(hWndParent))
    {
        return FALSE;
    }

    if (FloatingDocumentDock_IsClassName(hWndParent, L"__DockHostWindow"))
    {
        return TRUE;
    }

    if (FloatingDocumentDock_IsClassName(hWndParent, L"__FloatingWindowContainer"))
    {
        return TRUE;
    }

    if (FloatingDocumentDock_IsClassName(hWndParent, L"__WorkspaceContainer"))
    {
        return FloatingDocumentDock_IsClassName(GetParent(hWndParent), L"__DockHostWindow");
    }

    return FALSE;
}

BOOL FloatingDocumentDock_GetSourceContext(
    FloatingWindowContainer* pFloatingWindowContainer,
    POINT ptScreen,
    WorkspaceContainer** ppWorkspaceSource,
    int* pnSourceDocumentCount)
{
    return FloatingChildHost_GetDocumentSourceContext(
        pFloatingWindowContainer ? pFloatingWindowContainer->hWndChild : NULL,
        ptScreen,
        ppWorkspaceSource,
        pnSourceDocumentCount);
}

BOOL FloatingDocumentDock_HitTestTarget(
    FloatingWindowContainer* pFloatingWindowContainer,
    WorkspaceContainer* pWorkspaceSource,
    int nSourceDocuments,
    POINT ptScreen,
    WorkspaceDockTargetHit* pDockHit)
{
    if (!pWorkspaceSource || !pDockHit)
    {
        return FALSE;
    }

    memset(pDockHit, 0, sizeof(*pDockHit));

    WorkspaceContainer* pWorkspaceTarget = WorkspaceContainer_FindDropTargetAtScreenPoint(pWorkspaceSource, ptScreen);
    if (!pWorkspaceTarget || pWorkspaceTarget == pWorkspaceSource)
    {
        return FALSE;
    }

    HWND hWndTargetWorkspace = Window_GetHWND((Window*)pWorkspaceTarget);
    if (!hWndTargetWorkspace || !IsWindow(hWndTargetWorkspace))
    {
        return FALSE;
    }

    RECT rcTargetScreen = { 0 };
    GetWindowRect(hWndTargetWorkspace, &rcTargetScreen);
    int width = Win32_Rect_GetWidth(&rcTargetScreen);
    int height = Win32_Rect_GetHeight(&rcTargetScreen);
    if (width <= 0 || height <= 0)
    {
        return FALSE;
    }

    BOOL bSupportsSplitByHost = FloatingDocumentDock_IsWorkspaceSplitDockSupported(
        pFloatingWindowContainer,
        hWndTargetWorkspace);
    int nSourceDocs = nSourceDocuments;
    if (nSourceDocs <= 0)
    {
        nSourceDocs = WorkspaceContainer_GetViewportCount(pWorkspaceSource);
    }
    int nTargetDocs = WorkspaceContainer_GetViewportCount(pWorkspaceTarget);
    BOOL bSupportsSplit = WorkspaceDockPolicy_CanSplitTarget(
        nSourceDocs,
        nTargetDocs,
        bSupportsSplitByHost);

    POINT ptLocal = {
        ptScreen.x - rcTargetScreen.left,
        ptScreen.y - rcTargetScreen.top
    };

    RECT rcBounds = { 0, 0, width, height };
    RECT rcGuideCenter = { 0 };
    RECT rcGuideTop = { 0 };
    RECT rcGuideLeft = { 0 };
    RECT rcGuideRight = { 0 };
    RECT rcGuideBottom = { 0 };
    FloatingDocumentDock_GetClusterGuideRects(
        &rcBounds,
        width,
        height,
        &rcGuideCenter,
        &rcGuideTop,
        &rcGuideLeft,
        &rcGuideRight,
        &rcGuideBottom);

    int nDockSide = DKS_NONE;
    if (PtInRect(&rcGuideCenter, ptLocal))
    {
        nDockSide = DKS_CENTER;
    }
    else if (bSupportsSplit)
    {
        if (PtInRect(&rcGuideLeft, ptLocal))
        {
            nDockSide = DKS_LEFT;
        }
        else if (PtInRect(&rcGuideRight, ptLocal))
        {
            nDockSide = DKS_RIGHT;
        }
        else if (PtInRect(&rcGuideTop, ptLocal))
        {
            nDockSide = DKS_TOP;
        }
        else if (PtInRect(&rcGuideBottom, ptLocal))
        {
            nDockSide = DKS_BOTTOM;
        }
    }

    pDockHit->pWorkspaceTarget = pWorkspaceTarget;
    pDockHit->nDockSide = nDockSide;
    pDockHit->rcTargetScreen = rcTargetScreen;
    pDockHit->bSupportsSplit = bSupportsSplit;
    SetRectEmpty(&pDockHit->rcPreviewScreen);

    if (nDockSide != DKS_NONE)
    {
        RECT rcTargetClient = { 0 };
        GetClientRect(hWndTargetWorkspace, &rcTargetClient);
        RECT rcPreviewClient = rcTargetClient;

        if (nDockSide != DKS_CENTER)
        {
            DockLayout_GetDockPreviewRect(&rcTargetClient, nDockSide, &rcPreviewClient);
        }

        MapWindowPoints(hWndTargetWorkspace, NULL, (POINT*)&rcPreviewClient, 2);
        pDockHit->rcPreviewScreen = rcPreviewClient;
    }

    return TRUE;
}

BOOL FloatingDocumentDock_Attempt(
    FloatingWindowContainer* pFloatingWindowContainer,
    BOOL bForceMainWorkspace)
{
    if (!pFloatingWindowContainer)
    {
        return FALSE;
    }

    POINT ptCursor = { 0 };
    GetCursorPos(&ptCursor);

    WorkspaceContainer* pWorkspaceSource = NULL;
    int nSourceDocuments = 0;
    if (!FloatingDocumentDock_GetSourceContext(
        pFloatingWindowContainer,
        ptCursor,
        &pWorkspaceSource,
        &nSourceDocuments))
    {
        return FALSE;
    }

    WorkspaceDockTargetHit workspaceDockHit = { 0 };
    if (bForceMainWorkspace)
    {
        workspaceDockHit.pWorkspaceTarget = PanitentApp_GetWorkspaceContainer(PanitentApp_Instance());
        workspaceDockHit.nDockSide = DKS_CENTER;
        workspaceDockHit.bSupportsSplit = FALSE;
    }
    else if (!FloatingDocumentDock_HitTestTarget(
        pFloatingWindowContainer,
        pWorkspaceSource,
        nSourceDocuments,
        ptCursor,
        &workspaceDockHit))
    {
        return FALSE;
    }

    if (!workspaceDockHit.pWorkspaceTarget || workspaceDockHit.pWorkspaceTarget == pWorkspaceSource)
    {
        return FALSE;
    }

    return DocumentDockTransition_DockSourceToWorkspace(
        Window_GetHWND((Window*)pFloatingWindowContainer),
        &pFloatingWindowContainer->hWndChild,
        workspaceDockHit.pWorkspaceTarget,
        workspaceDockHit.nDockSide,
        pFloatingWindowContainer->iDockSizeHint);
}

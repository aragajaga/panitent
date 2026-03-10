#include "precomp.h"

#include "dockhost.h"

#include "dockhostmetrics.h"
#include "toolwndframe.h"
#include "win32/util.h"
#include "win32/window.h"

BOOL DockData_GetClientRect(DockData* pDockData, RECT* rc)
{
    if (!pDockData)
    {
        return FALSE;
    }

    RECT rcClient = pDockData->rc;
    if (pDockData->bShowCaption)
    {
        rcClient.top += DockHostMetrics_GetCaptionHeight() + 1;
    }

    if (DockNodeRole_IsRoot(pDockData->nRole, pDockData->lpszName))
    {
        int iLeft = 0, iRight = 0, iTop = 0, iBottom = 0;
        DockHostMetrics_GetRootGutters(&iLeft, &iRight, &iTop, &iBottom);
        rcClient.left += iLeft;
        rcClient.right -= iRight;
        rcClient.top += iTop;
        rcClient.bottom -= iBottom;
        if (rcClient.left > rcClient.right)
        {
            rcClient.left = rcClient.right;
        }
        if (rcClient.top > rcClient.bottom)
        {
            rcClient.top = rcClient.bottom;
        }
    }

    *rc = rcClient;
    return TRUE;
}

void DockData_Init(DockData* pDockData)
{
    pDockData->iGripPos = 64;
    pDockData->dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
    pDockData->bShowCaption = FALSE;
    pDockData->bCollapsed = FALSE;
    pDockData->hWndActiveTab = NULL;
    pDockData->nRole = DOCK_ROLE_UNKNOWN;
    pDockData->nPaneKind = DOCK_PANE_NONE;
    pDockData->nDockSide = DKS_NONE;
    pDockData->uModelNodeId = 0;
    pDockData->nViewId = PNT_DOCK_VIEW_NONE;
}

BOOL DockData_GetCaptionRect(DockData* pDockData, RECT* rc)
{
    if (!pDockData || !rc || !pDockData->bShowCaption)
    {
        return FALSE;
    }

    CaptionFrameMetrics metrics = { 0 };
    metrics.borderSize = 3;
    metrics.captionHeight = DockHostMetrics_GetCaptionHeight();
    metrics.buttonSpacing = 3;
    metrics.textPaddingLeft = 3;
    metrics.textPaddingRight = 3;
    metrics.textPaddingY = 0;
    CaptionFrameLayout layout = { 0 };
    if (!CaptionFrame_BuildLayout(&pDockData->rc, &metrics, NULL, 0, &layout))
    {
        return FALSE;
    }

    *rc = layout.rcCaption;
    return TRUE;
}

BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect)
{
    if (!pDockHostWindow || !pRect)
    {
        return FALSE;
    }

    HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    GetClientRect(hWnd, pRect);
    TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
    if (pRoot && pRoot->data)
    {
        DockData* pRootData = (DockData*)pRoot->data;
        RECT rcRootClient = { 0 };
        if (DockData_GetClientRect(pRootData, &rcRootClient))
        {
            *pRect = rcRootClient;
        }
    }

    return Win32_Rect_GetWidth(pRect) > 0 && Win32_Rect_GetHeight(pRect) > 0;
}

#include "precomp.h"

#include "dockhostarrange.h"

#include "dockhostautohide.h"
#include "dockhostbinding.h"
#include "dockhostgeometry.h"
#include "dockhostlayout.h"
#include "dockhostmetrics.h"
#include "dockhostzone.h"
#include "dockinspectordialog.h"
#include "win32/util.h"
#include "win32/window.h"

void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow)
{
    TreeNode* pRoot = pDockHostWindow->pRoot_;
    if (!pRoot)
    {
        return;
    }

    int iLeft = 0, iRight = 0, iTop = 0, iBottom = 0;
    DockHostZone_Sync(
        pDockHostWindow,
        DockHostMetrics_GetZoneTabGutter(),
        &iLeft,
        &iRight,
        &iTop,
        &iBottom);
    DockHostMetrics_SetRootGutters(iLeft, iRight, iTop, iBottom);
    DockHostLayout_AssignRects(pRoot, DockHostMetrics_GetBorderWidth(), 96);

    PostOrderTreeTraversal traversal = { 0 };
    PostOrderTreeTraversal_Init(&traversal, pRoot);
    TreeNode* pCurrentNode = NULL;
    while (pCurrentNode = PostOrderTreeTraversal_GetNext(&traversal))
    {
        DockData* pDockData = (DockData*)pCurrentNode->data;
        if (pDockData && pDockData->hWnd)
        {
            if (pDockHostWindow->fAutoHideOverlayVisible &&
                pDockHostWindow->hWndAutoHideOverlayHost &&
                IsWindow(pDockHostWindow->hWndAutoHideOverlayHost) &&
                pDockData->hWnd == pDockHostWindow->hWndAutoHideOverlay &&
                GetParent(pDockData->hWnd) == pDockHostWindow->hWndAutoHideOverlayHost)
            {
                continue;
            }

            RECT rc = { 0 };
            DockData_GetClientRect(pDockData, &rc);
            if (!DockHostWindow_IsWorkspaceWindow(pDockData->hWnd))
            {
                Win32_ContractRect(&rc, 4, 4);
            }

            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;
            if (width <= 12 || height <= 12)
            {
                ShowWindow(pDockData->hWnd, SW_HIDE);
            }
            else {
                ShowWindow(pDockData->hWnd, SW_SHOWNA);
                SetWindowPos(
                    pDockData->hWnd,
                    NULL,
                    rc.left,
                    rc.top,
                    width,
                    height,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            }
        }
    }

    PostOrderTreeTraversal_Destroy(&traversal);

    DockHostWindow_UpdateAutoHideOverlay(pDockHostWindow);
    Window_Invalidate((Window*)pDockHostWindow);
    DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, pDockHostWindow->pRoot_);
}

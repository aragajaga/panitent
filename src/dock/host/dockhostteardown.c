#include "precomp.h"

#include "dockhostteardown.h"

#include "dockhostautohide.h"
#include "dockhostinput.h"
#include "dockinspectordialog.h"
#include "win32/window.h"

static BOOL DockHostTeardown_ShouldPreserveHwnd(HWND hWnd, const HWND* phWndPreserve, int cPreserve)
{
    if (!hWnd || !phWndPreserve || cPreserve <= 0)
    {
        return FALSE;
    }

    for (int i = 0; i < cPreserve; ++i)
    {
        if (phWndPreserve[i] == hWnd)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void DockHostTeardown_DestroyTreeRecursive(TreeNode* pNode, const HWND* phWndPreserve, int cPreserve)
{
    if (!pNode)
    {
        return;
    }

    DockHostTeardown_DestroyTreeRecursive(pNode->node1, phWndPreserve, cPreserve);
    DockHostTeardown_DestroyTreeRecursive(pNode->node2, phWndPreserve, cPreserve);

    DockData* pDockData = (DockData*)pNode->data;
    if (pDockData && pDockData->hWnd && IsWindow(pDockData->hWnd) &&
        !DockHostTeardown_ShouldPreserveHwnd(pDockData->hWnd, phWndPreserve, cPreserve))
    {
        DestroyWindow(pDockData->hWnd);
    }

    free(pNode->data);
    free(pNode);
}

void DockHostWindow_DestroyNodeTree(TreeNode* pRootNode, const HWND* phWndPreserve, int cPreserve)
{
    DockHostTeardown_DestroyTreeRecursive(pRootNode, phWndPreserve, cPreserve);
}

void DockHostWindow_ClearLayout(DockHostWindow* pDockHostWindow, const HWND* phWndPreserve, int cPreserve)
{
    if (!pDockHostWindow)
    {
        return;
    }

    DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
    DockHostInput_ClearCaptionState(pDockHostWindow);
    DockHostWindow_ClearAutoHideCaptionState(pDockHostWindow);
    DockHostWindow_DestroyNodeTree(pDockHostWindow->pRoot_, phWndPreserve, cPreserve);
    pDockHostWindow->pRoot_ = NULL;
    if (pDockHostWindow->m_pDockInspectorDialog)
    {
        DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, NULL);
    }
    Window_Invalidate((Window*)pDockHostWindow);
}

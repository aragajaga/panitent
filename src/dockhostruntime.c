#include "precomp.h"

#include "dockhostruntime.h"

#include "dockgroup.h"
#include "dockhostautohide.h"
#include "dockhostinput.h"
#include "dockhostlayout.h"
#include "dockhostmetrics.h"
#include "dockhostpaint.h"
#include "dockhostzone.h"
#include "dockinspectordialog.h"
#include "dockviewcatalog.h"
#include "oledroptarget.h"
#include "panitentapp.h"
#include "theme.h"
#include "toolwndframe.h"
#include "win32/window.h"
#include "win32/util.h"
#include "win32/windowmap.h"

static BOOL DockHostRuntime_ShouldPreserveHwnd(HWND hWnd, const HWND* phWndPreserve, int cPreserve)
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

static BOOL DockHostRuntime_OnDropFiles(void* pContext, HDROP hDrop)
{
    UNREFERENCED_PARAMETER(pContext);
    return PanitentApp_OpenDroppedFiles(PanitentApp_Instance(), hDrop);
}

BOOL DockHostWindow_OnCreate(DockHostWindow* pDockHostWindow, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(lpcs);

    HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
    DockHostWindow_RefreshTheme(pDockHostWindow);
    OleFileDropTarget_Register(
        hWnd,
        DockHostRuntime_OnDropFiles,
        pDockHostWindow,
        (IDropTarget**)&pDockHostWindow->pFileDropTarget);
    return TRUE;
}

void DockHostWindow_OnDestroy(DockHostWindow* pDockHostWindow)
{
    HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);

    if (pDockHostWindow->pFileDropTarget)
    {
        OleFileDropTarget_Revoke(hWnd, (IDropTarget**)&pDockHostWindow->pFileDropTarget);
    }
    DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
    if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
    {
        DestroyWindow(pDockHostWindow->hWndAutoHideOverlayHost);
    }
    pDockHostWindow->hWndAutoHideOverlayHost = NULL;
    if (pDockHostWindow->hCaptionBrush_)
    {
        DeleteObject(pDockHostWindow->hCaptionBrush_);
        pDockHostWindow->hCaptionBrush_ = NULL;
    }
}

void DockHostWindow_OnPaint(DockHostWindow* pDockHostWindow)
{
    HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    int width = Win32_Rect_GetWidth(&rcClient);
    int height = Win32_Rect_GetHeight(&rcClient);

    HDC hdcTarget = hdc;
    HDC hdcBuffer = NULL;
    HBITMAP hbmBuffer = NULL;
    HGDIOBJ hOldBitmap = NULL;
    if (width > 0 && height > 0)
    {
        hdcBuffer = CreateCompatibleDC(hdc);
        if (hdcBuffer)
        {
            hbmBuffer = CreateCompatibleBitmap(hdc, width, height);
            if (hbmBuffer)
            {
                hOldBitmap = SelectObject(hdcBuffer, hbmBuffer);
                hdcTarget = hdcBuffer;
            }
        }
    }

    DockHostZone_SyncHostGutters(pDockHostWindow, DockHostMetrics_GetZoneTabGutter());
    DockHostPaint_PaintContent(
        pDockHostWindow,
        hdcTarget,
        &rcClient,
        pDockHostWindow->hCaptionBrush_,
        DockHostMetrics_GetZoneTabGutter(),
        DockHostMetrics_GetBorderWidth());

    if (hdcTarget == hdcBuffer)
    {
        BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);
        SelectObject(hdcBuffer, hOldBitmap);
        DeleteObject(hbmBuffer);
        DeleteDC(hdcBuffer);
    }
    else if (hdcBuffer)
    {
        DeleteDC(hdcBuffer);
    }

    EndPaint(hWnd, &ps);
}

void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(state);

    if (pDockHostWindow->pRoot_ && pDockHostWindow->pRoot_->data)
    {
        RECT rcRoot = { 0 };
        rcRoot.right = cx;
        rcRoot.bottom = cy;
        ((DockData*)pDockHostWindow->pRoot_->data)->rc = rcRoot;

        DockHostWindow_Rearrange(pDockHostWindow);
    }
}

BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd)
{
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    WCHAR szClassNameBuf[64] = L"";
    GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
    return wcscmp(szClassNameBuf, L"__WorkspaceContainer") == 0;
}

static void DockHostRuntime_AssignPersistentNameForHWND(DockData* pDockData, HWND hWnd)
{
    WCHAR szClassNameBuf[64] = L"";
    WCHAR szTitleBuf[MAX_PATH] = L"";
    PanitentDockViewId nViewId;
    PCWSTR pszCanonicalName;

    if (!pDockData || !hWnd || !IsWindow(hWnd))
    {
        return;
    }

    GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
    GetWindowTextW(hWnd, szTitleBuf, ARRAYSIZE(szTitleBuf));
    nViewId = PanitentDockViewCatalog_FindForWindow(szClassNameBuf, szTitleBuf);
    pszCanonicalName = PanitentDockViewCatalog_GetCanonicalName(nViewId);
    if (pszCanonicalName && pszCanonicalName[0] != L'\0')
    {
        pDockData->nViewId = nViewId;
        wcscpy_s(pDockData->lpszName, MAX_PATH, pszCanonicalName);
        return;
    }

    if (pDockData->lpszCaption[0] != L'\0')
    {
        wcscpy_s(pDockData->lpszName, MAX_PATH, pDockData->lpszCaption);
    }
}

static void DockHostRuntime_DestroyTreeRecursive(TreeNode* pNode, const HWND* phWndPreserve, int cPreserve)
{
    if (!pNode)
    {
        return;
    }

    DockHostRuntime_DestroyTreeRecursive(pNode->node1, phWndPreserve, cPreserve);
    DockHostRuntime_DestroyTreeRecursive(pNode->node2, phWndPreserve, cPreserve);

    DockData* pDockData = (DockData*)pNode->data;
    if (pDockData && pDockData->hWnd && IsWindow(pDockData->hWnd) &&
        !DockHostRuntime_ShouldPreserveHwnd(pDockData->hWnd, phWndPreserve, cPreserve))
    {
        DestroyWindow(pDockData->hWnd);
    }

    free(pNode->data);
    free(pNode);
}

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

void DockHostWindow_DestroyNodeTree(TreeNode* pRootNode, const HWND* phWndPreserve, int cPreserve)
{
    DockHostRuntime_DestroyTreeRecursive(pRootNode, phWndPreserve, cPreserve);
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

DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd)
{
    WCHAR szClassNameBuf[64] = L"";
    if (!hWnd || !IsWindow(hWnd))
    {
        return DOCK_PANE_NONE;
    }

    if (DockHostWindow_IsWorkspaceWindow(hWnd))
    {
        return DOCK_PANE_DOCUMENT;
    }

    GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
    if (wcscmp(szClassNameBuf, L"__DockHostWindow") == 0)
    {
        Window* pWindow = WindowMap_Get(hWnd);
        DockHostWindow* pDockHostWindow = (DockHostWindow*)pWindow;
        TreeNode* pRoot = pDockHostWindow ? DockHostWindow_GetRoot(pDockHostWindow) : NULL;
        if (DockGroup_NodeContainsPaneKind(pRoot, DOCK_PANE_DOCUMENT))
        {
            return DOCK_PANE_DOCUMENT;
        }
    }

    return DOCK_PANE_TOOL;
}

void DockData_PinHWND(DockHostWindow* pDockHostWindow, DockData* pDockData, HWND hWnd)
{
    if (!pDockHostWindow || !pDockData || !hWnd || !IsWindow(hWnd))
    {
        return;
    }

    GetWindowText(hWnd, pDockData->lpszCaption, MAX_PATH);
    SetParent(hWnd, Window_GetHWND((Window*)pDockHostWindow));

    DWORD dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_POPUP);
    dwStyle |= WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);

    pDockData->bShowCaption = !DockHostWindow_IsWorkspaceWindow(hWnd);
    pDockData->hWnd = hWnd;
    if (DockHostWindow_IsWorkspaceWindow(hWnd))
    {
        pDockData->nRole = DOCK_ROLE_WORKSPACE;
        pDockData->nPaneKind = DOCK_PANE_DOCUMENT;
    }
    else if (pDockData->nRole == DOCK_ROLE_UNKNOWN)
    {
        pDockData->nRole = DOCK_ROLE_PANEL;
        pDockData->nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    }
    else if (pDockData->nPaneKind == DOCK_PANE_NONE)
    {
        pDockData->nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    }

    if (pDockData->lpszName[0] == L'\0')
    {
        DockHostRuntime_AssignPersistentNameForHWND(pDockData, hWnd);
    }
}

void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window)
{
    HWND hWnd = Window_GetHWND(window);
    DockData_PinHWND(pDockHostWindow, pDockData, hWnd);
}

void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow)
{
    PanitentThemeColors colors = { 0 };
    HWND hWnd = NULL;

    if (!pDockHostWindow)
    {
        return;
    }

    PanitentTheme_GetColors(&colors);
    if (pDockHostWindow->hCaptionBrush_)
    {
        DeleteObject(pDockHostWindow->hCaptionBrush_);
    }
    pDockHostWindow->hCaptionBrush_ = CreateSolidBrush(colors.accent);

    hWnd = Window_GetHWND((Window*)pDockHostWindow);
    if (hWnd && IsWindow(hWnd))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
    {
        RedrawWindow(
            pDockHostWindow->hWndAutoHideOverlayHost,
            NULL,
            NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
}

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

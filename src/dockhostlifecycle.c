#include "precomp.h"

#include "dockhostruntime.h"

#include "dockhostautohide.h"
#include "dockhostinput.h"
#include "dockhostpaint.h"
#include "dockhostmetrics.h"
#include "dockhostzone.h"
#include "dockinspectordialog.h"
#include "oledroptarget.h"
#include "panitentapp.h"
#include "resource.h"
#include "win32/window.h"
#include "win32/util.h"

#define IDM_DOCKINSPECTOR 101

static BOOL DockHostLifecycle_OnDropFiles(void* pContext, HDROP hDrop)
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
        DockHostLifecycle_OnDropFiles,
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

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (LOWORD(wParam))
    {
    case IDM_DOCKINSPECTOR:
        DockHostInput_InvokeInspectorDialog(pDockHostWindow);
        return TRUE;
    }

    return FALSE;
}

LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE:
        DockHostInput_OnMouseMove(
            pDockHostWindow,
            (int)(short)GET_X_LPARAM(lParam),
            (int)(short)GET_Y_LPARAM(lParam),
            (UINT)wParam,
            DockHostMetrics_GetZoneTabGutter());
        return 0;

    case WM_MOUSELEAVE:
        DockHostInput_OnMouseLeave(pDockHostWindow);
        return 0;

    case WM_LBUTTONDOWN:
        DockHostInput_OnLButtonDown(
            pDockHostWindow,
            FALSE,
            (int)(short)GET_X_LPARAM(lParam),
            (int)(short)GET_Y_LPARAM(lParam),
            (UINT)wParam,
            DockHostMetrics_GetZoneTabGutter());
        return 0;

    case WM_LBUTTONUP:
        DockHostInput_OnLButtonUp(
            pDockHostWindow,
            (int)(short)GET_X_LPARAM(lParam),
            (int)(short)GET_Y_LPARAM(lParam),
            (UINT)wParam);
        return 0;

    case WM_CAPTURECHANGED:
        DockHostInput_OnCaptureChanged(pDockHostWindow);
        return 0;

    case WM_CONTEXTMENU:
        DockHostInput_OnContextMenu(
            pDockHostWindow,
            (HWND)wParam,
            (int)(short)GET_X_LPARAM(lParam),
            (int)(short)GET_Y_LPARAM(lParam));
        return 0;
    }

    return Window_UserProcDefault((Window*)pDockHostWindow, hWnd, message, wParam, lParam);
}

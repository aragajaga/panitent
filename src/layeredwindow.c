#include "precomp.h"
#include "win32/window.h"
#include "layeredwindow.h"

#include "resource.h"

static const WCHAR szClassName[] = L"__LayeredWindow";

void LayeredWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_OWNDC;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)NULL;
    lpwcex->lpszClassName = szClassName;
}

void LayeredWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"LayeredWindow";
    lpcs->style = WS_POPUP | WS_VISIBLE;
    lpcs->x = 0;
    lpcs->y = 0;
    lpcs->cx = 184;
    lpcs->cy = 184;
}

enum {
    DOCK_TOPPANE,
    DOCK_TOPSPLIT,
    DOCK_LEFTPANE,
    DOCK_LEFTSPLIT,
    DOCK_CENTER,
    DOCK_RIGHTSPLIT,
    DOCK_RIGHTPANE,
    DOCK_BOTTOMSPLIT,
    DOCK_BOTTOMPANE
};

BLENDFUNCTION g_blendFunction = {
    .BlendOp = AC_SRC_OVER,
    .BlendFlags = 0,
    .SourceConstantAlpha = 255,
    .AlphaFormat = AC_SRC_ALPHA
};

void LayeredWindow_DrawDockIcon(LayeredWindow* pLayeredWindow, HDC hdcDest, int x, int y, int idx)
{
    HDC hdcScreen = GetDC(NULL);
    HDC hdcOverlay = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCK_ICONS));
    HGDIOBJ hOldObj = SelectObject(hdcOverlay, hBitmap);

    AlphaBlend(hdcDest, x, y, 24, 24, hdcOverlay, idx * 24, 0, 24, 24, g_blendFunction);
    SelectObject(hdcOverlay, hOldObj);
    DeleteObject(hBitmap);
    DeleteDC(hdcOverlay);
    ReleaseDC(NULL, hdcScreen);
}

void LayeredWindow_DrawDockButton(LayeredWindow* pLayeredWindow, HDC hdcDest, int x, int y, int idx)
{
    HDC hdcScreen = GetDC(NULL);
    HDC hdcOverlay = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCK_BTNFRAME));
    HGDIOBJ hOldObj = SelectObject(hdcOverlay, hBitmap);

    AlphaBlend(hdcDest, x, y, 32, 32, hdcOverlay, idx * 32, 0, 32, 32, g_blendFunction);
    SelectObject(hdcOverlay, hOldObj);
    DeleteObject(hBitmap);
    DeleteDC(hdcOverlay);
    ReleaseDC(NULL, hdcScreen);
}


void LayeredWindow_Draw(LayeredWindow* pLayeredWindow)
{
    HWND hWnd = Window_GetHWND(pLayeredWindow);

    HDC hdcScreen = GetDC(NULL);

    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BLENDFUNCTION blendFunction = { 0 };
    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.SourceConstantAlpha = 255;
    blendFunction.AlphaFormat = AC_SRC_ALPHA;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, 184, 184);
    HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);

    // Draw background
    {
        HDC hdcBackground = CreateCompatibleDC(hdcScreen);
        HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCK_SUGGEST));
        HGDIOBJ hOldObj = SelectObject(hdcBackground, hBitmap);

        AlphaBlend(hdcMem, 0, 0, 184, 184, hdcBackground, 0, 0, 184, 184, blendFunction);
        SelectObject(hdcBackground, hOldObj);
        DeleteObject(hBitmap);
        DeleteDC(hdcBackground);
    }

    int x = 0;
    int y = 0;
    switch (pLayeredWindow->iHover)
    {
    case DOCK_TOPPANE:
        x = 76;
        y = 4;
        break;
    case DOCK_TOPSPLIT:
        x = 76;
        y = 40;
        break;
    case DOCK_LEFTPANE:
        x = 4;
        y = 76;
        break;
    case DOCK_LEFTSPLIT:
        x = 40;
        y = 76;
        break;
    case DOCK_CENTER:
        x = 76;
        y = 76;
        break;
    case DOCK_RIGHTSPLIT:
        x = 112;
        y = 76;
        break;
    case DOCK_RIGHTPANE:
        x = 148;
        y = 76;
        break;
    case DOCK_BOTTOMSPLIT:
        x = 76;
        y = 112;
        break;
    case DOCK_BOTTOMPANE:
        x = 76;
        y = 148;
        break;
    }

    if (pLayeredWindow->iHover != -1)
    {
        LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, x, y, 1);
    }
    
    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 76, 4, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 80, 8, DOCK_TOPPANE);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 76, 40, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 80, 44, DOCK_TOPSPLIT);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 4, 76, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 8, 80, DOCK_LEFTPANE);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 40, 76, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 44, 80, DOCK_LEFTSPLIT);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 76, 76, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 80, 80, DOCK_CENTER);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 112, 76, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 116, 80, DOCK_RIGHTSPLIT);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 148, 76, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 152, 80, DOCK_RIGHTPANE);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 76, 112, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 80, 116, DOCK_BOTTOMSPLIT);

    LayeredWindow_DrawDockButton(pLayeredWindow, hdcMem, 76, 148, 0);
    LayeredWindow_DrawDockIcon(pLayeredWindow, hdcMem, 80, 152, DOCK_BOTTOMPANE);

    SIZE sizeSplash = { 184, 184 };

    POINT ptZero = { 0 };
    HMONITOR hmonPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorinfo = { 0 };
    monitorinfo.cbSize = sizeof(monitorinfo);
    GetMonitorInfo(hmonPrimary, &monitorinfo);

    POINT ptOrigin;
    ptOrigin.x = monitorinfo.rcWork.left + (monitorinfo.rcWork.right - monitorinfo.rcWork.left - sizeSplash.cx) / 2;
    ptOrigin.y = monitorinfo.rcWork.top + (monitorinfo.rcWork.bottom - monitorinfo.rcWork.top - sizeSplash.cy) / 2;

    // AlphaBlend(hScreenDC, 0, 0, 256, 256, hdcSrc, 0, 0, 256, 256, blendFunction);

    POINT ptPos = { 0, 0 };
    SIZE sizeWnd = { sizeSplash.cx, sizeSplash.cy };
    POINT ptSrc = { 0, 0 };
    UpdateLayeredWindow(hWnd, hdcScreen, &ptOrigin, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blendFunction, ULW_ALPHA);

    // SetLayeredWindowAttributes(hWnd, 0, 254, LWA_ALPHA);

    SelectObject(hdcMem, hOldObj);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

int LayeredWindow_DockPosHitTest(LayeredWindow* pLayeredWindow, int x, int y)
{
    if (x >= 76 && x < 76 + 32 && y >= 4 && y < 4 + 32)
    {
        return DOCK_TOPPANE;
    }
    else if (x >= 76 && x < 76 + 32 && y >= 40 && y < 40 + 32)
    {
        return DOCK_TOPSPLIT;
    }
    else if (x >= 4 && x < 4 + 32 && y >= 76 && y < 76 + 32)
    {
        return DOCK_LEFTPANE;
    }
    else if (x >= 40 && x < 40 + 32 && y >= 76 && y < 76 + 32)
    {
        return DOCK_LEFTSPLIT;
    }
    else if (x >= 76 && x < 76 + 32 && y >= 76 && y < 76 + 32)
    {
        return DOCK_CENTER;
    }
    else if (x >= 112 && x < 112 + 32 && y >= 76 && y < 76 + 32)
    {
        return DOCK_RIGHTSPLIT;
    }
    else if (x >= 148 && x < 148 + 32 && y >= 76 && y < 76 + 32)
    {
        return DOCK_RIGHTPANE;
    }
    else if (x >= 76 && x < 76 + 32 && y >= 112 && y < 112 + 32)
    {
        return DOCK_BOTTOMSPLIT;
    }
    else if (x >= 76 && x < 76 + 32 && y >= 148 && y < 148 + 32)
    {
        return DOCK_BOTTOMPANE;
    }

    return -1;
}

void LayeredWindow_OnMouseMove(LayeredWindow* pLayeredWindow, int x, int y)
{
    int hitTestVal = LayeredWindow_DockPosHitTest(pLayeredWindow, x, y);
    pLayeredWindow->iHover = hitTestVal;

    LayeredWindow_Draw(pLayeredWindow);
}

void LayeredWindow_OnLButtonUp(LayeredWindow* pLayeredWindow, int x, int y)
{
    LayeredWindow_DockPosHitTest(pLayeredWindow, x, y);
}

LRESULT LayeredWindow_UserProc(LayeredWindow* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEMOVE:
        LayeredWindow_OnMouseMove(window, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONUP:
        LayeredWindow_OnLButtonUp(window, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        return 0;
    }

    return Window_UserProcDefault(window, hWnd, message, wParam, lParam);
}

BOOL LayeredWindow_OnCreate(LayeredWindow* pLayeredWindow, LPCREATESTRUCT lpcs)
{
    LayeredWindow_Draw(pLayeredWindow);

    return TRUE;
}

void LayeredWindow_OnDestroy(LayeredWindow* window)
{
}

void LayeredWindow_OnPaint(LayeredWindow* window)
{
    HWND hWnd = Window_GetHWND(window);

    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);

    EndPaint(hWnd, &ps);
}

void LayeredWindow_Init(LayeredWindow* pLayeredWindow)
{
    Window_Init(&pLayeredWindow->base);

    pLayeredWindow->base.szClassName = szClassName;

    pLayeredWindow->base.OnCreate = (FnWindowOnCreate)LayeredWindow_OnCreate;
    pLayeredWindow->base.OnDestroy = (FnWindowOnDestroy)LayeredWindow_OnDestroy;
    pLayeredWindow->base.OnPaint = (FnWindowOnPaint)LayeredWindow_OnPaint;

    _WindowInitHelper_SetPreRegisterRoutine((Window *)pLayeredWindow, (FnWindowPreRegister)LayeredWindow_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window *)pLayeredWindow, (FnWindowPreCreate)LayeredWindow_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window *)pLayeredWindow, (FnWindowUserProc)LayeredWindow_UserProc);

    pLayeredWindow->iHover = -1;
}

LayeredWindow* LayeredWindow_Create()
{
    LayeredWindow* pLayeredWindow = (LayeredWindow*)malloc(sizeof(LayeredWindow));
    memset(pLayeredWindow, 0, sizeof(LayeredWindow));

    if (pLayeredWindow)
    {
        LayeredWindow_Init(pLayeredWindow);
    }

    return pLayeredWindow;
}

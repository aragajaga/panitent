#include "precomp.h"

#include "floatingdocumenthost.h"

#include "floatingwindowcontainer.h"
#include "win32/window.h"
#include "win32/util.h"

BOOL FloatingDocumentHost_CreatePinnedWindow(
    DockHostWindow* pDockHostTarget,
    HWND hWndChild,
    const RECT* pWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut)
{
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }

    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FALSE;
    }

    FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
    HWND hWndFloating = pFloatingWindowContainer ? Window_CreateWindow((Window*)pFloatingWindowContainer, NULL) : NULL;
    if (!pFloatingWindowContainer || !hWndFloating || !IsWindow(hWndFloating))
    {
        free(pFloatingWindowContainer);
        return FALSE;
    }

    FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostTarget);
    FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_DOCUMENT);
    FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndChild);

    RECT rcWindow = { 0 };
    if (pWindowRect)
    {
        rcWindow = *pWindowRect;
    }
    else {
        GetWindowRect(hWndFloating, &rcWindow);
    }

    int width = max(1, Win32_Rect_GetWidth(&rcWindow));
    int height = max(1, Win32_Rect_GetHeight(&rcWindow));
    SetWindowPos(
        hWndFloating,
        HWND_TOP,
        rcWindow.left,
        rcWindow.top,
        width,
        height,
        SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);

    if (bStartMove)
    {
        SetForegroundWindow(hWndFloating);
        SendMessage(hWndFloating, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(ptMoveScreen.x, ptMoveScreen.y));
    }

    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

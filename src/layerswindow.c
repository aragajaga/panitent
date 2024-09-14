#include "precomp.h"

#include "win32/window.h"

#include "LayersWindow.h"

static const WCHAR szClassName[] = L"__LayersWindow";

/* Private forward declarations */
LayersWindow* LayersWindow_Create();
void LayersWindow_Init(LayersWindow*);

void LayersWindow_PreRegister(LPWNDCLASSEX lpwcex);
void LayersWindow_PreCreate(LPCREATESTRUCT lpcs);

BOOL LayersWindow_OnCreate(LayersWindow* pLayersWindow, LPCREATESTRUCT lpcs);
void LayersWindow_PostCreate(LayersWindow* pLayersWindow);
void LayersWindow_OnSize(LayersWindow* pLayersWindow, UINT state, int cx, int cy);
void LayersWindow_OnPaint(LayersWindow* pLayersWindow);
void LayersWindow_OnLButtonUp(LayersWindow* pLayersWindow, int x, int y);
void LayersWindow_OnRButtonUp(LayersWindow* pLayersWindow, int x, int y);
void LayersWindow_OnContextMenu(LayersWindow* pLayersWindow, int x, int y);
BOOL LayersWindow_OnCommand(LayersWindow* pLayersWindow, WPARAM wParam, LPARAM lParam);
void LayersWindow_OnDestroy(LayersWindow* pLayersWindow);
LRESULT LayersWindow_UserProc(LayersWindow* pLayersWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LayersWindow* LayersWindow_Create()
{
    LayersWindow* pLayersWindow = (LayersWindow *)malloc(sizeof(LayersWindow));
    memset(pLayersWindow, 0, sizeof(LayersWindow));

    if (pLayersWindow)
    {
        LayersWindow_Init(pLayersWindow);
    }

    return pLayersWindow;
}

void LayersWindow_Init(LayersWindow* pLayersWindow)
{
    Window_Init(&pLayersWindow->base);

    pLayersWindow->base.szClassName = szClassName;

    pLayersWindow->base.OnCreate = (FnWindowOnCreate)LayersWindow_OnCreate;
    pLayersWindow->base.OnSize = (FnWindowOnSize)LayersWindow_OnSize;
    pLayersWindow->base.OnDestroy = (FnWindowOnDestroy)LayersWindow_OnDestroy;
    pLayersWindow->base.OnPaint = (FnWindowOnPaint)LayersWindow_OnPaint;
    pLayersWindow->base.PostCreate = (FnWindowPostCreate)LayersWindow_PostCreate;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pLayersWindow, (FnWindowPreRegister)LayersWindow_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pLayersWindow, (FnWindowPreCreate)LayersWindow_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pLayersWindow, (FnWindowUserProc)LayersWindow_UserProc);
}

void LayersWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void LayersWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"LayersWindow";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

BOOL LayersWindow_OnCreate(LayersWindow* pLayersWindow, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(pLayersWindow);
    UNREFERENCED_PARAMETER(lpcs);

    return TRUE;
}

void LayersWindow_PostCreate(LayersWindow* pLayersWindow)
{
    pLayersWindow->m_pListBoxCtl = ListBoxCtl_Create();

    HWND hButton = CreateWindowEx(0, WC_BUTTON, L"+", WS_VISIBLE | WS_CHILD, 2, 2, 20, 20, Window_GetHWND((Window*)pLayersWindow), NULL, GetModuleHandle(NULL), NULL);

    HWND hListBox = NULL;
    hListBox = Window_CreateWindow((Window*)pLayersWindow->m_pListBoxCtl, Window_GetHWND((Window*)pLayersWindow));

    ListBox_AddString(hListBox, L"Layer 1");
    ListBox_AddString(Window_GetHWND((Window*)pLayersWindow->m_pListBoxCtl), L"Layer 2");
    ListBox_AddString(Window_GetHWND((Window*)pLayersWindow->m_pListBoxCtl), L"Layer 3");
}

void LayersWindow_OnSize(LayersWindow* pLayersWindow, UINT state, int cx, int cy)
{
    if (pLayersWindow->m_pListBoxCtl)
    {
        HWND hListBox = Window_GetHWND((Window*)pLayersWindow->m_pListBoxCtl);
        int i = (int)SendMessage(hListBox, LB_ADDSTRING, 0, L"Roaster");
        i = (int)SendMessage(hListBox, LB_ADDSTRING, 0, L"Roaster");


        ListBox_AddString(Window_GetHWND((Window*)pLayersWindow->m_pListBoxCtl), L"Layer 2");

        SetWindowPos(Window_GetHWND((Window*)pLayersWindow->m_pListBoxCtl), NULL, 0, 24, cx, cy-24, 0);
    }
}

void LayersWindow_OnPaint(LayersWindow* pLayersWindow)
{
    HWND hwnd = pLayersWindow->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void LayersWindow_OnLButtonUp(LayersWindow* pLayersWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(pLayersWindow);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void LayersWindow_OnRButtonUp(LayersWindow* pLayersWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(pLayersWindow);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void LayersWindow_OnContextMenu(LayersWindow* pLayersWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(pLayersWindow);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

BOOL LayersWindow_OnCommand(LayersWindow* pLayersWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pLayersWindow);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    return TRUE;
}

void LayersWindow_OnDestroy(LayersWindow* pLayersWindow)
{
    UNREFERENCED_PARAMETER(pLayersWindow);
}

LRESULT LayersWindow_UserProc(LayersWindow* pLayersWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_RBUTTONUP:
        LayersWindow_OnRButtonUp(pLayersWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        LayersWindow_OnLButtonUp(pLayersWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        LayersWindow_OnContextMenu(pLayersWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault(pLayersWindow, hWnd, message, wParam, lParam);
}

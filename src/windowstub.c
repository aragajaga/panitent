#include "precomp.h"

#include "win32/window.h"

#include "windowstub.h"

static const WCHAR szClassName[] = L"__WindowStub";

/* Private forward declarations */
WindowStub* WindowStub_Create(struct Application* app);
void WindowStub_Init(WindowStub*, struct Application* app);

void WindowStub_PreRegister(LPWNDCLASSEX lpwcex);
void WindowStub_PreCreate(LPCREATESTRUCT lpcs);

BOOL WindowStub_OnCreate(WindowStub* pWindowStub, LPCREATESTRUCT lpcs);
void WindowStub_OnPaint(WindowStub* pWindowStub);
void WindowStub_OnLButtonUp(WindowStub* pWindowStub, int x, int y);
void WindowStub_OnRButtonUp(WindowStub* pWindowStub, int x, int y);
void WindowStub_OnContextMenu(WindowStub* pWindowStub, int x, int y);
void WindowStub_OnDestroy(WindowStub* pWindowStub);
LRESULT WindowStub_UserProc(WindowStub* pWindowStub, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

WindowStub* WindowStub_Create(struct Application* app)
{
    WindowStub* pWindowStub = (WindowStub *)calloc(1, sizeof(WindowStub));

    if (pWindowStub)
    {
        WindowStub_Init(pWindowStub, app);
    }

    return pWindowStub;
}

void WindowStub_Init(WindowStub* pWindowStub, struct Application* app)
{
    Window_Init(&pWindowStub->base, app);

    pWindowStub->base.szClassName = szClassName;

    pWindowStub->base.OnCreate = (FnWindowOnCreate)WindowStub_OnCreate;
    pWindowStub->base.OnDestroy = (FnWindowOnDestroy)WindowStub_OnDestroy;
    pWindowStub->base.OnPaint = (FnWindowOnPaint)WindowStub_OnPaint;

    _WindowInitHelper_SetPreRegisterRoutine((Window *)pWindowStub, (FnWindowPreRegister)WindowStub_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window *)pWindowStub, (FnWindowPreCreate)WindowStub_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window *)pWindowStub, (FnWindowUserProc)WindowStub_UserProc);
}

void WindowStub_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE);
    lpwcex->lpszClassName = szClassName;
}

void WindowStub_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"WindowStub";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

BOOL WindowStub_OnCreate(WindowStub* pWindowStub, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(pWindowStub);
    UNREFERENCED_PARAMETER(lpcs);

    return TRUE;
}

void WindowStub_OnPaint(WindowStub* pWindowStub)
{
    HWND hwnd = pWindowStub->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void WindowStub_OnLButtonUp(WindowStub* pWindowStub, int x, int y)
{
    UNREFERENCED_PARAMETER(pWindowStub);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WindowStub_OnRButtonUp(WindowStub* pWindowStub, int x, int y)
{
    UNREFERENCED_PARAMETER(pWindowStub);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WindowStub_OnContextMenu(WindowStub* pWindowStub, int x, int y)
{
    UNREFERENCED_PARAMETER(pWindowStub);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WindowStub_OnCommand(WindowStub* pWindowStub, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pWindowStub);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
}

void WindowStub_OnDestroy(WindowStub* pWindowStub)
{
    UNREFERENCED_PARAMETER(pWindowStub);
}

LRESULT WindowStub_UserProc(WindowStub* pWindowStub, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_RBUTTONUP:
        WindowStub_OnRButtonUp(pWindowStub, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        WindowStub_OnLButtonUp(pWindowStub, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        WindowStub_OnContextMenu(pWindowStub, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault(pWindowStub, hWnd, message, wParam, lParam);
}

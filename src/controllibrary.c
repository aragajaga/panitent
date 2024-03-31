#include "precomp.h"
#include "win32/window.h"
#include "controllibrary.h"

static const WCHAR szClassName[] = L"__ControlLibrary";

/* Private forward declarations */
ControlLibrary* ControlLibrary_Create(struct Application*);
void ControlLibrary_Init(ControlLibrary*, struct Application*);

void ControlLibrary_PreRegister(LPWNDCLASSEX);
void ControlLibrary_PreCreate(LPCREATESTRUCT);

BOOL ControlLibrary_OnCreate(ControlLibrary*, LPCREATESTRUCT);
void ControlLibrary_OnPaint(ControlLibrary*);
void ControlLibrary_OnLButtonUp(ControlLibrary*, int, int);
void ControlLibrary_OnRButtonUp(ControlLibrary*, int, int);
void ControlLibrary_OnContextMenu(ControlLibrary*, int, int);
void ControlLibrary_OnDestroy(ControlLibrary*);
LRESULT CALLBACK ControlLibrary_UserProc(ControlLibrary*, HWND hWnd, UINT, WPARAM, LPARAM);

ControlLibrary* ControlLibrary_Create(struct Application* app)
{
    ControlLibrary* window = calloc(1, sizeof(ControlLibrary));

    if (window)
    {
        ControlLibrary_Init(window, app);
    }

    return window;
}

void ControlLibrary_Init(ControlLibrary* window, struct Application* app)
{
    Window_Init(&window->base, app);

    window->base.szClassName = szClassName;

    window->base.OnCreate = ControlLibrary_OnCreate;
    window->base.OnDestroy = ControlLibrary_OnDestroy;
    window->base.OnPaint = ControlLibrary_OnPaint;
    window->base.PreRegister = ControlLibrary_PreRegister;
    window->base.PreCreate = ControlLibrary_PreCreate;
    window->base.UserProc = ControlLibrary_UserProc;
}

void ControlLibrary_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE);
    lpwcex->lpszClassName = szClassName;
}

BOOL ControlLibrary_OnCreate(ControlLibrary* window, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lpcs);
}

void ControlLibrary_OnPaint(ControlLibrary* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    Rectangle(hdc, 0, 0, 32, 32);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void ControlLibrary_OnLButtonUp(ControlLibrary* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void ControlLibrary_OnRButtonUp(ControlLibrary* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void ControlLibrary_OnContextMenu(ControlLibrary* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void ControlLibrary_OnCommand(ControlLibrary* window, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
}

void ControlLibrary_OnDestroy(ControlLibrary* window)
{
    UNREFERENCED_PARAMETER(window);
}

LRESULT CALLBACK ControlLibrary_UserProc(ControlLibrary* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_RBUTTONUP:
        ControlLibrary_OnRButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        ControlLibrary_OnLButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        ControlLibrary_OnContextMenu(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault(window, hWnd, message, wParam, lParam);
}

void ControlLibrary_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"ControlLibrary";
    lpcs->style = WS_CAPTION | WS_OVERLAPPED | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

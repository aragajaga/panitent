#include "window.h"

#include "windowmap.h"

static const WCHAR szClassName[] = L"DummyWindowClass";

Window* Window_Create(struct Application*);
void Window_Init(Window*, struct Application*);

void Window_PreRegister(LPWNDCLASSEX lpwcex);
void Window_PreCreate(LPCREATESTRUCT lpcs);

BOOL Window_OnCreate(Window* pWindow, LPCREATESTRUCT lpcs);
void Window_OnPaint(Window* pWindow);
BOOL Window_OnClose(Window* pWindow);
void Window_OnDestroy(Window* pWindow);
BOOL Window_OnCommand(Window* pWindow, WPARAM wParam, LPARAM lParam);
void Window_OnSize(Window* pWindow, int cx, int cy);
void Window_Subclass(Window* pWindow, HWND hWnd);
LRESULT CALLBACK Window_UserProc(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Window_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL Window_Register(Window*);
HWND Window_CreateWindow(Window*, HWND);

Window* Window_Create(struct Application* app)
{
    Window* window = (Window*)malloc(sizeof(Window));


    if (window)
    {
        memset(window, 0, sizeof(Window));
        Window_Init(window, app);
    }

    return window;
}

void Window_Init(Window* pWindow, struct Application* app)
{
    pWindow->app = app;

    pWindow->szClassName = szClassName;

    pWindow->PreRegister = (FnWindowPreRegister)Window_PreRegister;
    pWindow->PreCreate = (FnWindowPreCreate)Window_PreCreate;

    pWindow->OnCreate = (FnWindowPostCreate)Window_OnCreate;
    pWindow->PostCreate = (FnWindowPostCreate)Window_PostCreate;
    pWindow->OnPaint = (FnWindowOnPaint)Window_OnPaint;
    pWindow->OnClose = (FnWindowOnClose)Window_OnClose;
    pWindow->OnDestroy = (FnWindowOnDestroy)Window_OnDestroy;
    pWindow->OnCommand = (FnWindowOnCommand)Window_OnCommand;
    pWindow->OnSize = (FnWindowOnSize)Window_OnSize;

    pWindow->UserProc = (FnWindowUserProc)Window_UserProc;
}

void Window_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->cbSize = sizeof(WNDCLASSEX);
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->lpfnWndProc = (WNDPROC)Window_WndProc;
    lpwcex->cbWndExtra = sizeof(Window*);
    lpwcex->hInstance = GetModuleHandle(NULL);
    lpwcex->hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void Window_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Dummy Window";
    lpcs->style = WS_OVERLAPPEDWINDOW;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = CW_USEDEFAULT;
    lpcs->cy = CW_USEDEFAULT;
    lpcs->hwndParent = NULL;
    lpcs->hInstance = GetModuleHandle(NULL);
    lpcs->lpCreateParams = (LPVOID)NULL;
}

BOOL Window_OnCreate(Window* pWindow, LPCREATESTRUCT lpcs)
{
    return TRUE;
}

void Window_PostCreate(Window* pWindow)
{
}

void Window_OnPaint(Window* pWindow)
{
    /*
    PAINTSTRUCT ps;
    HDC hdc;
    
    hdc = BeginPaint(window->hWnd, &ps);
    
    RECT rcClient = { 0 };
    GetClientRect(window->hWnd, &rcClient);
    
    SetBkMode(hdc, TRANSPARENT);
    
    const WCHAR szDummyText[] = L"This is a dummy window created by passing default overloads";
    size_t nDummyTextLen = wcslen(szDummyText);
    
    DrawTextEx(hdc, szDummyText, nDummyTextLen, &rcClient, DT_SINGLELINE | DT_CENTER | DT_VCENTER, NULL);
    
    EndPaint(window->hWnd, &ps);
    */
}

BOOL Window_OnClose(Window* pWindow)
{
    /* Do nothing */
    return FALSE;
}

void Window_OnDestroy(Window* pWindow)
{
    PostQuitMessage(0);
}

BOOL Window_OnCommand(Window* pWindow, WPARAM wParam, LPARAM lParam)
{
    return FALSE; /* pass trough */
}

void Window_OnSize(Window* pWindow, int cx, int cy)
{
    /* Do nothing */
}

LRESULT Window_UserProcDefault(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_CREATE:
        return pWindow->OnCreate(pWindow, (LPCREATESTRUCT)lParam);

    case WM_PAINT:
        if (!pWindow->pfnPrevWindowProc)
        {
            pWindow->OnPaint(pWindow);
            return 0;
        }

    case WM_CLOSE:
        if (pWindow->OnClose(pWindow))
        {
            return 0;
        }
        break;

    case WM_DESTROY:
        pWindow->OnDestroy(pWindow);
        return 0;
        break;

    case WM_COMMAND:
        if (pWindow->OnCommand(pWindow, wParam, lParam))
        {
            return 0;
        }
        break;

    case WM_SIZE:
        pWindow->OnSize(pWindow, (UINT)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        return 0;
        break;
    }

    if (pWindow && pWindow->pfnPrevWindowProc)
    {
        return CallWindowProc(pWindow->pfnPrevWindowProc, pWindow->hWnd, message, wParam, lParam);
    }
    else {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

LRESULT Window_UserProc(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return Window_UserProcDefault(pWindow, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Window_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* pWindow = NULL;
    pWindow = WindowMap_Get(hWnd);
    if (!pWindow)
    {
        if (pWndCreating)
        {
            WindowMap_Insert(hWnd, pWndCreating);
            pWndCreating->hWnd = hWnd;
        }
    }

    if (pWindow)
    {
        return pWindow->UserProc(pWindow, hWnd, message, wParam, lParam);
    }
    else {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

BOOL Window_Register(Window* pWindow)
{
    WNDCLASSEX wcex = { 0 };
    BOOL bRegistered = FALSE;

    bRegistered = GetClassInfoEx(GetModuleHandle(NULL), pWindow->szClassName, &wcex);

    if (!bRegistered && pWindow->PreRegister)
    {
        pWindow->PreRegister(&pWindow->wcex);
        pWindow->wcex.cbSize = sizeof(WNDCLASSEX);
        pWindow->wcex.lpfnWndProc = (WNDPROC)Window_WndProc;
        pWindow->wcex.cbWndExtra = sizeof(Window*);
        pWindow->wcex.hInstance = GetModuleHandle(NULL);
        return (BOOL)RegisterClassEx(&pWindow->wcex);
    }

    return TRUE;
}

HWND Window_CreateWindow(Window* pWindow, HWND hParent)
{
    Window_Register(pWindow);

    LPWNDCLASSEX lpwcex = &pWindow->wcex;
    LPCREATESTRUCT cs = &pWindow->cs;

    pWindow->PreCreate(cs);
    if (lpwcex->lpszClassName)
    {
        cs->lpszClass = pWindow->wcex.lpszClassName;
    }

    if (hParent)
    {
        cs->style &= ~(WS_CAPTION | WS_THICKFRAME);
        cs->style |= WS_CHILD;
    }

    pWndCreating = pWindow;

    HWND hWindow = CreateWindowEx(
        cs->dwExStyle,
        cs->lpszClass,
        cs->lpszName,
        cs->style,
        cs->x,
        cs->y,
        cs->cx,
        cs->cy,
        hParent,
        cs->hMenu,
        lpwcex->hInstance,
        (LPVOID)NULL);

    WNDCLASSEX wcex = { 0 };
    GetClassInfoEx(lpwcex->hInstance, cs->lpszClass, &wcex);
    if (wcex.lpfnWndProc != (WNDPROC)Window_WndProc)
    {
        Window_Subclass(pWindow, hWindow);
        pWindow->OnCreate(pWindow, &pWindow->cs);
    }

    pWndCreating = NULL;

    pWindow->hWnd = hWindow;

    pWindow->PostCreate(pWindow);

    ShowWindow(hWindow, SW_NORMAL);
    UpdateWindow(hWindow);

    return hWindow;
}

void Window_Subclass(Window* pWindow, HWND hWnd)
{
    WNDPROC pfnCurrentWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);

    if (pfnCurrentWndProc != (WNDPROC)Window_WndProc)
    {
        pWindow->pfnPrevWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)Window_WndProc);
        pWindow->hWnd = hWnd;
    }
}

void Window_UnSubclass(Window* pWindow)
{
    WNDPROC pfnCurrentWndProc = (WNDPROC)GetWindowLongPtr(pWindow->hWnd, GWLP_WNDPROC);

    if (pfnCurrentWndProc == (WNDPROC)Window_WndProc)
    {
        SetWindowLongPtr(pWindow->hWnd, GWLP_WNDPROC, pWindow->pfnPrevWindowProc);
        pWindow->pfnPrevWindowProc = NULL;
    }
}

HWND Window_Detach(Window* pWindow)
{
    HWND hWnd = pWindow->hWnd;

    if (pWindow->pfnPrevWindowProc)
    {
        Window_UnSubclass(pWindow);
    }

    // RemoveFromHashMapByValue(&g_hWndMap, pWindow);

    pWindow->hWnd = NULL;

    return hWnd;
}

void Window_Attach(Window* pWindow, HWND hWnd)
{
    Window_Detach(pWindow);

    if (IsWindow(hWnd))
    {
        if (!WindowMap_Get(hWnd))
        {
            WindowMap_Insert(hWnd, pWindow);
            Window_Subclass(pWindow, hWnd);
        }
    }
}

LRESULT Window_SendMessage(Window* pWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    SendMessage(pWindow->hWnd, message, wParam, lParam);
}

void Window_SetTheme(Window* pWindow, PCWSTR lpszThemeName)
{
    SetWindowTheme(pWindow->hWnd, lpszThemeName, NULL);
}

DWORD Window_GetStyle(Window* pWindow)
{
    return (DWORD)GetWindowLongPtr(pWindow->hWnd, GWL_STYLE);
}

void Window_SetStyle(Window* pWindow, DWORD dwStyle)
{
    SetWindowLongPtr(pWindow->hWnd, GWL_STYLE, dwStyle);
}

HWND Window_GetHWND(Window* pWindow)
{
    return pWindow->hWnd;
}

void Window_GetClientRect(Window* pWindow, LPRECT lprc)
{
    GetClientRect(Window_GetHWND(pWindow), lprc);
}

void Window_Invalidate(Window* pWindow)
{
    InvalidateRgn(Window_GetHWND(pWindow), NULL, TRUE);
}

BOOL Window_Show(Window* pWindow, int nCmdShow)
{
    return ShowWindow(Window_GetHWND(pWindow), nCmdShow);
}

void _WindowInitHelper_SetPreRegisterRoutine(Window* pWindow, void(*pfnPreRegister)(LPWNDCLASSEX lpwcex))
{
    pWindow->PreRegister = pfnPreRegister;
}

void _WindowInitHelper_SetPreCreateRoutine(Window* pWindow, void(*pfnPreCreate)(LPCREATESTRUCT lpcs))
{
    pWindow->PreCreate = pfnPreCreate;
}

void _WindowInitHelper_SetUserProcRoutine(Window* pWindow, void(*pfnUserProc)(Window* pWindow, UINT message, WPARAM wParam, LPARAM lParam))
{
    pWindow->UserProc = pfnUserProc;
}

int Win32_Rect_GetWidth(LPRECT lprc)
{
    return lprc->right - lprc->left;
}

int Win32_Rect_GetHeight(LPRECT lprc)
{
    return lprc->bottom - lprc->top;
}

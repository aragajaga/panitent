#include "precomp.h"

#include "settings_wnd.h"

#include "win32/util.h"

static const WCHAR szClassName[] = L"Win32Ctl_SettingsWindow";

/* Private forward declarations */
SettingsWindow* SettingsWindow_Create();
void SettingsWindow_Init(SettingsWindow* pSettingsWindow);

void SettingsWindow_PreRegister(LPWNDCLASSEX lpwcex);
void SettingsWindow_PreCreate(LPCREATESTRUCT lpcs);

BOOL SettingsWindow_OnCreate(SettingsWindow* pSettingsWindow, LPCREATESTRUCT lpcs);
void SettingsWindow_OnPaint(SettingsWindow* pSettingsWindow);
void SettingsWindow_OnDestroy(SettingsWindow* pSettingsWindow);
LRESULT SettingsWindow_UserProc(SettingsWindow* pSettingsWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

SettingsWindow* SettingsWindow_Create()
{
    SettingsWindow* pSettingsWindow = (SettingsWindow*)malloc(sizeof(SettingsWindow));
    if (pSettingsWindow)
    {
        SettingsWindow_Init(pSettingsWindow);
    }

    return pSettingsWindow;
}

void SettingsWindow_Init(SettingsWindow* pSettingsWindow)
{
    memset(pSettingsWindow, 0, sizeof(SettingsWindow));

    Window_Init(&pSettingsWindow->base);

    pSettingsWindow->base.szClassName = szClassName;
    
    pSettingsWindow->base.OnCreate = (FnWindowOnCreate)SettingsWindow_OnCreate;
    pSettingsWindow->base.OnDestroy = (FnWindowOnDestroy)SettingsWindow_OnDestroy;
    pSettingsWindow->base.OnPaint = (FnWindowOnPaint)SettingsWindow_OnPaint;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pSettingsWindow, (FnWindowPreRegister)SettingsWindow_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pSettingsWindow, (FnWindowPreCreate)SettingsWindow_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pSettingsWindow, (FnWindowUserProc)SettingsWindow_UserProc);
}

void SettingsWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void SettingsWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    RECT rcClient = { 0 };
    rcClient.right = 640;
    rcClient.bottom = 480;
    AdjustWindowRect(&rcClient, WS_OVERLAPPEDWINDOW, FALSE);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Settings";
    lpcs->x = (screenWidth - RECTWIDTH(&rcClient)) / 2;
    lpcs->y = (screenHeight - RECTHEIGHT(&rcClient)) / 2;
    lpcs->cx = RECTWIDTH(&rcClient);
    lpcs->cy = RECTHEIGHT(&rcClient);
}

BOOL SettingsWindow_OnCreate(SettingsWindow* pSettingsWindow, LPCREATESTRUCT lpcs)
{
    /* TODO */
}

void SettingsWindow_OnPaint(SettingsWindow* pSettingsWindow)
{
    /* Do nothing */
}

void SettingsWindow_OnDestroy(SettingsWindow* pSettingsWindow)
{
    /* Do nothing */
}

void SettingsWindow_OnSize(SettingsWindow* pSettingsWindow, UINT state, int width, int height)
{
    /* TODO */
}

void SettingsWindow_OnNotify(SettingsWindow* pSettingsWindow, int idFrom, LPNMHDR pnmhdr)
{
    /* TODO */
}

void SettingsWindow_OnCommand(SettingsWindow* pSettingsWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED)
    {
        Window_Destroy((Window *)pSettingsWindow);
    }
}

void SettingsWindow_OnGetMinMaxInfo(SettingsWindow* pSettingsWindow, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pSettingsWindow);
    UNREFERENCED_PARAMETER(wParam);

    RECT rcClient = { 0 };
    rcClient.right = 640;
    rcClient.bottom = 480;
    AdjustWindowRect(&rcClient, WS_OVERLAPPEDWINDOW, FALSE);

    LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
    lpmmi->ptMinTrackSize.x = RECTWIDTH(&rcClient);
    lpmmi->ptMinTrackSize.y = RECTHEIGHT(&rcClient);
}

LRESULT SettingsWindow_UserProc(SettingsWindow* pSettingsWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        SettingsWindow_OnSize(pSettingsWindow, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_NOTIFY:
        SettingsWindow_OnNotify(pSettingsWindow, (int)wParam, (LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        SettingsWindow_OnCommand(pSettingsWindow, wParam, lParam);
        break;

    case WM_GETMINMAXINFO:
        SettingsWindow_OnGetMinMaxInfo(pSettingsWindow, wParam, lParam);
        break;
    }

    return Window_UserProcDefault(pSettingsWindow, hWnd, message, wParam, lParam);
}

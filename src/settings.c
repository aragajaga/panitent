#include "stdafx.h"
#include "log.h"
#include "settings.h"
#include <windowsx.h>
#include <knownfolders.h>

static ATOM settings_window_class;

struct {
    HWND canvas_border_checkbox;
} settings_window_handles;

int access_settings_file()
{
    PWSTR appdata_path = NULL;
    
    HRESULT hr = SHGetKnownFolderPath(
            &FOLDERID_RoamingAppData,
            KF_FLAG_DEFAULT,
            NULL,
            &appdata_path);

    if (S_OK == hr)
    {
        WCHAR app_dir[MAX_PATH];
        
        wsprintf(app_dir, L"%s\\%s", appdata_path, L"panitent");
        CoTaskMemFree(appdata_path);
        
        CreateDirectory(app_dir, NULL);
    }
}

int register_settings_window_class()
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc      = (WNDPROC)wndproc_settings_window;
    wc.hInstance        = GetModuleHandle(NULL);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName    = L"Win32Ctl_SettingsWindow";

    if (!RegisterClass(&wc))
    {
        logger("[SettingsWindow] Failed to register window class");
        return 0;
    }
}

int show_settings_window(HWND parent_hwnd)
{
    if (!settings_window_class)
        settings_window_class = register_settings_window_class();
    
    if (settings_window_class)
    {
        RECT rc = {0};
        rc.right    = 640;
        rc.bottom   = 480;
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        
        CreateWindow(
                settings_window_class,
                L"Settings",
                WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2,
                (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2,
                rc.right - rc.left, rc.bottom - rc.top,
                parent_hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL);
    }
}

HWND hTabControl;
HWND hButton;
HWND hButtonCancel;

LRESULT CALLBACK wndproc_settings_window(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        initialize_settings_window(hwnd, msg);
        break;
    case WM_GETMINMAXINFO:
    {
        RECT rc = {0};
        rc.right    = 640;
        rc.bottom   = 480;
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = rc.right - rc.left;
        lpMMI->ptMinTrackSize.y = rc.bottom - rc.top;
    }
        break;
    case WM_SIZE:
        SetWindowPos(hTabControl, HWND_TOP, 10, 10, GET_X_LPARAM(lParam)-20, GET_Y_LPARAM(lParam)-60, 0);
        SetWindowPos(hButton, HWND_TOP, GET_X_LPARAM(lParam)-110, GET_Y_LPARAM(lParam)-40, 100, 30, 0);
        SetWindowPos(hButtonCancel, HWND_TOP, GET_X_LPARAM(lParam)-220, GET_Y_LPARAM(lParam)-40, 100, 30, 0);
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int initialize_settings_window(HWND hwnd)
{
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    HFONT hFont = CreateFontIndirect(&ncm.lfMessageFont);
    
    hTabControl = CreateWindowEx(
            0,
            WC_TABCONTROL,
            NULL,
            WS_CHILD | WS_VISIBLE,
            10, 10,
            620, 180,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        
    
    TCITEM tab_main = {0};
    tab_main.mask       = TCIF_TEXT;
    tab_main.pszText    = L"Main";
    tab_main.iImage     = -1;
    
    TCITEM tab_secondary = {0};
    tab_secondary.mask       = TCIF_TEXT;
    tab_secondary.pszText    = L"Secondary";
    tab_secondary.iImage     = -1;
    
    SendMessage(hTabControl, TCM_INSERTITEM, (WPARAM)0, (LPARAM)&tab_main);
    SendMessage(hTabControl, TCM_INSERTITEM, (WPARAM)1, (LPARAM)&tab_secondary);
    
    SendMessage(hTabControl, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    
    RECT tab_view_rc    = {0};
    tab_view_rc.left    = 10;
    tab_view_rc.top     = 10;
    tab_view_rc.right   = 640;
    tab_view_rc.bottom  = 200;
    
    SendMessage(hTabControl, TCM_ADJUSTRECT, (WPARAM)FALSE, (LPARAM)&tab_view_rc);    
    HWND hTabView = CreateWindow(
            L"STATIC",
            NULL,
            WS_CHILD | WS_VISIBLE,
            tab_view_rc.left,
            tab_view_rc.top,
            tab_view_rc.right - tab_view_rc.left,
            tab_view_rc.bottom - tab_view_rc.top,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        
    settings_window_handles.canvas_border_checkbox = CreateWindow(
            L"BUTTON",
            L"Show border",
            WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
            10, 10,
            100, 20,
            hTabView,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    SendMessage(
            settings_window_handles.canvas_border_checkbox,
            WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    
    hButton = CreateWindow(
            L"BUTTON",
            L"Apply",
            WS_CHILD | WS_VISIBLE,
            10, 340,
            100, 30,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    
    hButtonCancel = CreateWindow(
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE,
            120, 340,
            100, 30,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    SendMessage(hButtonCancel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
}

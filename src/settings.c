#include "stdafx.h"
#include "log.h"
#include "settings.h"

static ATOM settings_window_class;

struct {
    HWND canvas_border_checkbox;
} settings_window_handles;

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

LRESULT CALLBACK wndproc_settings_window(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        initialize_settings_window(hwnd, msg);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
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

    settings_window_handles.canvas_border_checkbox = CreateWindow(
            L"BUTTON",
            L"Show border",
            WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
            10, 10,
            100, 20,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    SendMessage(
            settings_window_handles.canvas_border_checkbox,
            WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    
    HWND hButton = CreateWindow(
            L"BUTTON",
            L"Button",
            WS_CHILD | WS_VISIBLE,
            10, 40,
            100, 30,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
}

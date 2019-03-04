#include "stdafx.h"
#include "settings.h"
#include <windowsx.h>
#include <knownfolders.h>

#define LPSZ_SETTINGSWINDOW L"Win32Ctl_SettingsWindow"

ATOM RegisterSettingsWindowClass()
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc      = (WNDPROC)SettingsWndProc;
    wc.hInstance        = GetModuleHandle(NULL);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName    = LPSZ_SETTINGSWINDOW;

    return RegisterClass(&wc);
}

ATOM settings_window_class;

int ShowSettingsWindow(HWND parent_hwnd)
{
    if (!settings_window_class)
        settings_window_class = RegisterSettingsWindowClass();
    
    if (settings_window_class)
    {
        RECT rc = {0};
        rc.right    = 640;
        rc.bottom   = 480;
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        
        CreateWindow(
                LPSZ_SETTINGSWINDOW,
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
    
    return 0;
}

HWND hTabControl;
HWND hButtonCancel;
HWND hButtonOk;

HWND hTPMain;
HWND hTPDebug;

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR pNm = (LPNMHDR)lParam;
            
            if (pNm->hwndFrom == hTabControl && pNm->code == TCN_SELCHANGE)
            {
                UINT tabId = TabCtrl_GetCurSel(hTabControl);
                
                switch (tabId)
                {
                case IDT_SETTINGS_PAGE_MAIN:
                    ShowWindow(hTPMain, SW_NORMAL);
                    ShowWindow(hTPDebug, SW_HIDE);
                    break;
                case IDT_SETTINGS_PAGE_DEBUG:
                    ShowWindow(hTPMain, SW_HIDE);
                    ShowWindow(hTPDebug, SW_NORMAL);
                    MessageBox(NULL, L"Debug settings", L"Warning", MB_OK);
                    break;
                }
            }
        }
        break;
    case WM_CREATE:
        InitSettingsWindow(hwnd);
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
        {
        UINT width = GET_X_LPARAM(lParam);
        UINT height = GET_Y_LPARAM(lParam);
        
        SetWindowPos(hTabControl, HWND_TOP, 10, 10, width-20, height-60, 0);
        SetWindowPos(hButtonCancel, HWND_TOP, width-110, height-40, 100, 30, 0);
        SetWindowPos(hButtonOk, HWND_TOP, width-220, height-40, 100, 30, 0);
        
        RECT vrc;
        GetClientRect(hTabControl, &vrc);
        vrc.left += 10;
        vrc.top += 10;
        vrc.right += 10;
        vrc.bottom += 10;
        TabCtrl_AdjustRect(hTabControl, FALSE, &vrc);
        
        SetWindowPos(hTPMain,
                HWND_TOP,
                vrc.left, vrc.top,
                vrc.right-vrc.left, vrc.bottom-vrc.top,
                0);
                
        SetWindowPos(hTPDebug,
                HWND_TOP,
                vrc.left, vrc.top,
                vrc.right-vrc.left, vrc.bottom-vrc.top,
                0);
        }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void SetGuiFont(HWND hwnd)
{
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    HFONT hFont = CreateFontIndirect(&ncm.lfMessageFont);
    
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
}

int InitSettingsWindow(HWND hwnd)
{
    RegisterSettingsTabPageMain();
    RegisterSettingsTabPageDebug();
    
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
    SetGuiFont(hTabControl);
    
    TCITEM tab_main = {0};
    tab_main.mask       = TCIF_TEXT;
    tab_main.pszText    = L"Main";
    tab_main.iImage     = -1;
    TabCtrl_InsertItem(hTabControl, IDT_SETTINGS_PAGE_MAIN, &tab_main);
    
    TCITEM tab_debug = {0};
    tab_debug.mask       = TCIF_TEXT;
    tab_debug.pszText    = L"Debug";
    tab_debug.iImage     = -1;
    TabCtrl_InsertItem(hTabControl, IDT_SETTINGS_PAGE_DEBUG, &tab_debug);
    
    RECT tab_view_rc    = {0};
    tab_view_rc.left    = 10;
    tab_view_rc.top     = 10;
    tab_view_rc.right   = 640;
    tab_view_rc.bottom  = 200;   
    TabCtrl_AdjustRect(hTabControl, FALSE, &tab_view_rc);
    
    hTPMain = CreateWindow(
            L"SettingsTabPageMain",
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

    hTPDebug = CreateWindow(
            L"SettingsTabPageDebug",
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
    
    hButtonCancel = CreateWindow(
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE,
            420, 440,
            100, 30,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
    SetGuiFont(hButtonCancel);
    
    hButtonOk = CreateWindow(
            L"BUTTON",
            L"OK",
            WS_CHILD | WS_VISIBLE,
            530, 440,
            100, 30,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
    SetGuiFont(hButtonOk);
    
    return 0;
}

LRESULT CALLBACK SettingsTabPageMainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        {
        HWND hCheckBox = CreateWindow(
            L"BUTTON",
            L"Show border",
            WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
            10, 10,
            100, 20,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hCheckBox);
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void RegisterSettingsTabPageMain() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc      = (WNDPROC)SettingsTabPageMainProc;
    wc.hInstance        = GetModuleHandle(NULL);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName    = L"SettingsTabPageMain";
    
    RegisterClass(&wc);
}

LRESULT CALLBACK SettingsTabPageDebugProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        {
        HWND hCheckBox = CreateWindow(
            L"BUTTON",
            L"Log actions",
            WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
            10, 10,
            100, 20,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hCheckBox);
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void RegisterSettingsTabPageDebug() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc      = (WNDPROC)SettingsTabPageDebugProc;
    wc.hInstance        = GetModuleHandle(NULL);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName    = L"SettingsTabPageDebug";
    
    RegisterClass(&wc);
}

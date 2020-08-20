#include "precomp.h"

#include <shlwapi.h>
#include <stdio.h>

#include "new.h"
#include "debug.h"
#include "document.h"
#include "viewport.h"

void RegisterNewFileDialog()
{
    WNDCLASS wc = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = (WNDPROC)NewFileDialogWndProc;
    wc.hInstance        = GetModuleHandle(NULL);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName    = L"NewFileDialogClass";
    if (!RegisterClass(&wc))
        printf("[NewFileDialog] Class registration failed\n");
}

extern viewport_t g_viewport;

HWND hEditWidth;
HWND hEditHeight;
extern HWND hButtonOk;

LRESULT CALLBACK NewFileDialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDB_OK:
            {            
            WCHAR szWidth[40];
            WCHAR szHeight[40];
            int iWidth;
            int iHeight;
            
            GetWindowText(hEditWidth, szWidth, 40);
            GetWindowText(hEditHeight, szHeight, 40);
            
            iWidth = StrToInt(szWidth);
            iHeight = StrToInt(szHeight);
            
            printf("[NewFile] width: %d, height: %d\n", iWidth, iHeight);
            
            DestroyWindow(hwnd);
            document_new(iWidth, iHeight);
            }
            break;
        default:
            break;
        }
        break;
    case WM_SIZE:
        {
        WORD cx = LOWORD(lParam);
        WORD cy = HIWORD(lParam);
        SetWindowPos(hButtonOk, NULL, cx-110, cy-40, 100, 30, SWP_NOZORDER);
        }
        break;
    case WM_CREATE:
        {
        HWND hStaticWidth = CreateWindow(
            L"STATIC",
            L"Width:",
            WS_CHILD | WS_VISIBLE,
            10,
            15,
            70,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hStaticWidth);
    
        hEditWidth = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"640",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            100,
            10,
            100,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hEditWidth);
            
        HWND hStaticHeight = CreateWindow(
            L"STATIC",
            L"Height:",
            WS_CHILD | WS_VISIBLE,
            10,
            42,
            70,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hStaticHeight);
            
        hEditHeight = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"480",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            100,
            38,
            100,
            23,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hEditHeight);
    
        hButtonOk = CreateWindow(
            L"BUTTON",
            L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            200,
            200,
            100,
            30,
            hwnd,
            (HMENU)IDB_OK,
            GetModuleHandle(NULL),
            NULL);
        SetGuiFont(hButtonOk);
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }
    
    return 0;
}

void NewFileDialog(HWND hwnd)
{
    RegisterNewFileDialog();
    
    RECT rc = {0};
    rc.right    = 210;
    rc.bottom   = 120;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    
    CreateWindow(
            L"NewFileDialogClass",
            L"New",
            WS_VISIBLE | WS_OVERLAPPEDWINDOW,
            (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2,
            (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2,
            rc.right - rc.left, rc.bottom - rc.top,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
}

#include "new.h"
#include "debug.h"

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
        printf("[NewFileDialog] Class registration failed");
}

HWND hButtonOk;
LRESULT CALLBACK NewFileDialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDB_OK:
            DestroyWindow(hwnd);
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
    rc.right    = 600;
    rc.bottom   = 400;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    
    HWND hNewDlg = CreateWindow(
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

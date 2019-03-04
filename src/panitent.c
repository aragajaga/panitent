#include "viewport.h"
#include "toolshelf.h"
#include "file_open.h"
#include "settings.h"
#include "winuser.h"
#include "panitent.h"

#define IDS_STATUS 1330

static HINSTANCE hInstance;
static HWND hwndViewport;
static HWND hwndToolShelf;

HWND CreateStatusBar(HWND hParent)
{
    //TODO: Do check CommonContorls initialized
    HWND hStatusBar;
    RECT rcClient;
    
    hStatusBar = CreateWindowEx(
            0,
            STATUSCLASSNAME,
            NULL,
            SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            hParent,
            (HMENU)IDS_STATUS,
            GetModuleHandle(NULL),
            NULL);
     
    GetClientRect(hParent, &rcClient);

    return hStatusBar;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInstance = hInst;    
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);
    
    RegisterViewportCtl();
    RegisterToolShelf();
    
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu;
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_OPEN, L"&Open");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_SAVE, L"&Save");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_CLOSE, L"&Close");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&File");
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_UNDO, L"&Undo");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_REDO, L"&Redo");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_TESTFILL, L"&Test fill");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_CLRCANVAS, L"&Clear canvas");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_WU_LINES, L"&Wu lines");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Edit");
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_WINDOW_TOOLS, L"&Tools");
    AppendMenu(hSubMenu, MF_STRING, IDM_WINDOW_PALETTE, L"&Palette");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Window");
    CheckMenuItem(hSubMenu, IDM_WINDOW_TOOLS, MF_BYCOMMAND | MF_CHECKED);
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_OPTIONS_SETTINGS, L"&Settings");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Options");
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_TOPICS, L"Help &Topics");
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Help");
    
    /**************************************************************************/
    /************************** REGISTER WINDOW CLASS *************************/
    /**************************************************************************/
    
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC) WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"WindowClass";
    wcex.hIconSm = NULL;
    RegisterClassEx(&wcex);
    
    /**************************************************************************/
    
    HWND hwnd = CreateWindowEx(
        0,
        L"WindowClass",
        L"panit.ent",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    ShowWindow(hwnd, nCmdShow);
    SetMenu(hwnd, hMenu);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
        TranslateMessage(&msg);
    }
    
    UnregisterClasses();
    return (int) msg.wParam;
}

extern VIEWPORT vp;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_OPEN:
            FileOpen();
            break;
        case IDM_FILE_SAVE:
            FileSave();
            break;
        case IDM_FILE_CLOSE:
            PostQuitMessage(0);
            break;
        case IDM_EDIT_TESTFILL:
            CanvasFillTest(&vp.img);
            break;
        case IDM_EDIT_CLRCANVAS:
            CanvasFillSolid(&vp.img, 0x00ffffff);
            break;
        case IDM_EDIT_WU_LINES:
            CanvasWuLinesTest(&vp.img);
        case IDM_WINDOW_TOOLS:
            CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), IDM_WINDOW_TOOLS, IsWindowVisible(hwndToolShelf)?MF_UNCHECKED:MF_CHECKED);
            ShowWindow(hwndToolShelf, IsWindowVisible(hwndToolShelf)?SW_HIDE:SW_SHOW);
            break;
        case IDM_HELP_TOPICS:
            ShellExecute(hWnd, L"open", L"https://github.com/Aragajaga/panitent/wiki", 0, 0, SW_SHOWNORMAL);
            break;
        case IDM_OPTIONS_SETTINGS:
            ShowSettingsWindow(hWnd);
        default:
            break;
        }
        return 0;
    case WM_SIZE:
        {
            WORD cx = LOWORD(lParam);
            WORD cy = HIWORD(lParam);
            SetWindowPos(hwndViewport, NULL, 48, 0, cx-48, cy, SWP_NOZORDER);
        }
        return 0;
    case WM_CREATE:
        hwndViewport = CreateWindowEx(
                0,
                VIEWPORTCTL_WC,
                NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                0, 0, 0, 0,
                hWnd,
                (HMENU) VIEWPORTCTL_ID,
                hInstance,
                NULL);

        hwndToolShelf = CreateWindowEx(
                WS_EX_TOOLWINDOW,
                TOOLSHELF_WC,
                L"Tools",
                WS_VISIBLE | WS_CHILD,
                CW_USEDEFAULT, CW_USEDEFAULT,
                48, 256,
                hWnd,
                NULL,
                hInstance,
                NULL);
        
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;    
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void UnregisterClasses()
{
    UnregisterClass(VIEWPORTCTL_WC, NULL);
    UnregisterClass(TOOLSHELF_WC, NULL);
}

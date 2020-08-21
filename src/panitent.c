#include "precomp.h"

#include <commctrl.h>

#include "panitent.h"

#include "wu_primitives.h"
#include "option_bar.h"
#include "file_open.h"
#include "bresenham.h"
#include "toolshelf.h"
#include "dockhost.h"
#include "panitent.h"
#include "resource.h"
#include "settings.h"
#include "viewport.h"
#include "palette.h"
#include "winuser.h"
#include "new.h"

static HINSTANCE hInstance;
static HWND hwndToolShelf;

panitent_t g_panitent;

HFONT hFontSys;

void FetchSystemFont()
{
  NONCLIENTMETRICS ncm = {0};
  ncm.cbSize = sizeof(NONCLIENTMETRICS);

  BOOL bResult = SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
      sizeof(NONCLIENTMETRICS), &ncm, 0);
  if (!bResult)
  {
    return;
  }

  HFONT hFontNew = CreateFontIndirect(&ncm.lfMessageFont);
  if (!hFontNew)
  {
    return;
  }

  DeleteObject(hFontSys);
  hFontSys = hFontNew;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{   
    UNREFERENCED_PARAMETER(hPrevInstance)
    UNREFERENCED_PARAMETER(lpCmdLine)

    hInstance = hInst;    

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    FetchSystemFont();
    
    DockHost_Register(hInstance);
    RegisterToolShelf();
    register_palette_dialog(hInstance);
    option_bar_register_class(hInstance);


    bresenham_init();
    wu_init();
    g_primitives_context = g_wu_primitives;
    
    HMENU hMenu = CreateMainMenu();
    
    /* Регистрация класса главного окна приложения */
    WNDCLASSEX wcex = {0};
    wcex.cbSize         = sizeof(wcex);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC) WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName  = L"WindowClass";
    RegisterClassEx(&wcex);
    
    /* Создание главного окна приложения */
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
    g_panitent.hwnd_main = hwnd;

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
BOOL bConsoleAttached;

#ifdef _MSC_VER
int main()
{
    return WinMain(GetModuleHandle(NULL), NULL, NULL, 0);
}
#endif

HWND hPaletteToolbox;
HWND hwndDockHost;

HWND hwndToolbox;
HWND hwndPalette;

binary_tree_t* viewportNode;

void Panitent_DockHostInit(HWND hWnd, binary_tree_t* parent)
{
  parent->posFixedGrip = 128;
  parent->gripAlign = GRIP_ALIGN_END;
  parent->gripPosType = GRIP_POS_ABSOLUTE;
  parent->bShowCaption = TRUE;


  /* Working Area */
  binary_tree_t* nodeA = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodeB = calloc(1, sizeof(binary_tree_t));

  nodeA->posFixedGrip = 64;
  nodeA->gripAlign = GRIP_ALIGN_START;
  nodeA->gripPosType = GRIP_POS_ABSOLUTE;

  hwndPalette = CreateWindowEx(0, L"Win32Class_PaletteWindow", L"Palette",
      WS_CHILD | WS_VISIBLE, 0, 0, 128, 128, hWnd, NULL,
      GetModuleHandle(NULL), NULL);

  nodeB->lpszCaption = L"Palette";
  nodeB->bShowCaption = TRUE;
  nodeB->hwnd = hwndPalette;


  /* Toolbox and viewport split */
  binary_tree_t* nodeAA = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodeAB = calloc(1, sizeof(binary_tree_t));
  
  hwndToolbox = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLSHELF_WC, L"Tools",
      WS_VISIBLE | WS_CHILD, 0, 32, 64, 256, hWnd, NULL,
      GetModuleHandle(NULL), NULL);

  nodeAA->lpszCaption = L"Tool";
  nodeAA->bShowCaption = TRUE;
  nodeAA->hwnd = hwndToolbox;

  nodeAB->lpszCaption = L"Viewport";
  nodeAB->bShowCaption = TRUE;
  viewportNode = nodeAB;


  /* Set graph */
  nodeA->node1 = nodeAA;
  nodeA->node2 = nodeAB;

  parent->node1 = nodeA;
  parent->node2 = nodeB;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_NEW:
            NewFileDialog(hWnd);
            break;
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
            canvas_fill_solid(g_viewport.document->canvas, 0xFFFFFFFF);
            break;
        case IDM_EDIT_CLRCANVAS:
            canvas_clear(g_viewport.document->canvas);
            break;
        case IDM_EDIT_WU_LINES:
            MessageBox(NULL, L"Obsolete, sorry.", L"panit.ent", MB_OK);
            break;
        case IDM_WINDOW_TOOLS:
            CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), IDM_WINDOW_TOOLS, IsWindowVisible(hwndToolShelf)?MF_UNCHECKED:MF_CHECKED);
            ShowWindow(hwndToolShelf, IsWindowVisible(hwndToolShelf)?SW_HIDE:SW_SHOW);
            break;
        case IDM_HELP_TOPICS:
            ShellExecute(hWnd, L"open", L"https://github.com/Aragajaga/panitent/wiki", 0, 0, SW_SHOWNORMAL);
            break;
        case IDM_OPTIONS_SETTINGS:
            ShowSettingsWindow(hWnd);
            break;
        default:
            break;
        }
        break;
    case WM_SIZE:
        {
        WORD cx = LOWORD(lParam);
        WORD cy = HIWORD(lParam);

        SetWindowPos(hwndDockHost, NULL, 0, 0, cx, cy, SWP_NOACTIVATE |
            SWP_NOZORDER);

        /*
        if (g_viewport.win_handle)
        {
          SetWindowPos(g_viewport.win_handle, NULL, 64, 32, cx - 64, cy-32,
              SWP_NOACTIVATE | SWP_NOZORDER);
          SetWindowPos(g_option_bar.win_handle, NULL, 0, 0, cx, 32,
              SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
        */
        }
        break;
    case WM_CREATE:
        hwndDockHost = DockHost_Create(hWnd);

        RECT rcDockHost = {0};
        GetClientRect(hwndDockHost, &rcDockHost);
        root = calloc(1, sizeof(binary_tree_t));
        root->lpszCaption = L"Root";
        root->rc = rcDockHost;

        Panitent_DockHostInit(hwndDockHost, root);

        // option_bar_create(hWnd);
        
        break;
    case WM_THEMECHANGED:
        FetchSystemFont();
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    }
    
    return 0;
}

#if 0
HWND CreateStatusBar(HWND hParent)
{
    /* TODO: Do check CommonContorls initialized */
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
#endif

void SetGuiFont(HWND hwnd)
{
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFontSys, MAKELPARAM(FALSE, 0));
}

HMENU CreateMainMenu()
{
    HMENU hMenu;
    HMENU hSubMenu;
    
    hMenu = CreateMenu();
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_NEW, L"&New\tCtrl+N");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_OPEN, L"&Open\tCtrl+O");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");
    AppendMenu(hSubMenu, MF_STRING, IDM_FILE_CLOSE, L"&Close");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&File");
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_TESTFILL, L"&Test fill");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_CLRCANVAS, L"&Clear canvas");
    AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_WU_LINES, L"&Wu lines");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Edit");
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_WINDOW_TOOLS, L"&Tools");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Window");
    CheckMenuItem(hSubMenu, IDM_WINDOW_TOOLS, MF_BYCOMMAND | MF_CHECKED);
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_OPTIONS_SETTINGS, L"&Settings");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Options");
    
    hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_TOPICS, L"Help &Topics");
    AppendMenu(hSubMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Help");
    
    return hMenu;
}

void UnregisterClasses()
{
    UnregisterClass(VIEWPORTCTL_WC, NULL);
    UnregisterClass(TOOLSHELF_WC, NULL);
}

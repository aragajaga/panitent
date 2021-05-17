#include "precomp.h"

#include <commctrl.h>

#include "panitent.h"

#include "wu_primitives.h"
#include "option_bar.h"
#include "file_open.h"
#include "bresenham.h"
#include "toolbox.h"
#include "dockhost.h"
#include "docknode.h"
#include "panitent.h"
#include "resource.h"
#include "settings.h"
#include "viewport.h"
#include "palette.h"
#include "winuser.h"
#include "new.h"
#include "color_context.h"
#include "welcome.h"
#include "layers.h"

static HINSTANCE hInstance;
static HWND hwndToolShelf;

panitent_t g_panitent;

HFONT hFontSys;

const WCHAR szAppName[] = L"Panit.ent";

Document* g_activeDocument;

void FetchSystemFont()
{
  NONCLIENTMETRICS ncm = {0};
  ncm.cbSize           = sizeof(NONCLIENTMETRICS);

  BOOL bResult = SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                                      sizeof(NONCLIENTMETRICS),
                                      &ncm,
                                      0);
  if (!bResult) {
    return;
  }

  HFONT hFontNew = CreateFontIndirect(&ncm.lfMessageFont);
  if (!hFontNew) {
    return;
  }

  DeleteObject(hFontSys);
  hFontSys = hFontNew;
}

HFONT GetGuiFont()
{
  if (!hFontSys)
    FetchSystemFont();

  return hFontSys;
}

const WCHAR szAppWndClass[] = L"Win32Class_PanitentWnd";

BOOL AppWnd_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)WndProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = szAppWndClass;

  return RegisterClassEx(&wcex);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance)
  UNREFERENCED_PARAMETER(lpCmdLine)

  hInstance = hInst;

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_TAB_CLASSES;
  InitCommonControlsEx(&icex);

  FetchSystemFont();

  InitColorContext();
  InitDefaultTools();
  Layers_Register(hInstance);
  DockHost_Register(hInstance);
  Welcome_Register(hInstance);
  Viewport_Register(hInstance);
  Toolbox_Register(hInstance);
  register_palette_dialog(hInstance);
  option_bar_register_class(hInstance);
  SettingsWindow_Register(hInstance);

  bresenham_init();
  wu_init();
  g_primitives_context = g_wu_primitives;

  HMENU hMenu = CreateMainMenu();

  /* Регистрация класса главного окна приложения */
  AppWnd_RegisterClass(hInstance);

  /* Создание главного окна приложения */
  HWND hwnd = CreateWindowEx(0, szAppWndClass, szAppName, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
      hInstance, NULL);

  g_panitent.hwnd_main = hwnd;

  ShowWindow(hwnd, nCmdShow);
  SetMenu(hwnd, hMenu);

  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (GetMessage(&msg, NULL, 0, 0)) {
    DispatchMessage(&msg);
    TranslateMessage(&msg);
  }

  FreeColorContext();
  UnregisterClasses();
  return (int)msg.wParam;
}

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
HWND hwndOptionBar;
HWND hwndPalette;

void Panitent_DockHostInit(HWND hwnd)
{
  DockContainer* root = DockContainer_Create(64, E_DIRECTION_VERTICAL, E_ALIGN_END);
  const DockHost* host = DockHost_Create(hwnd, (DockBase*)root);

  DockContainer* split1 = DockContainer_Create(196, E_DIRECTION_HORIZONTAL, E_ALIGN_END);
  DockContainer* split2 = DockContainer_Create(196, E_DIRECTION_VERTICAL, E_ALIGN_START);
  DockContainer* split3 = DockContainer_Create(72, E_DIRECTION_HORIZONTAL, E_ALIGN_START);

  DockWindow* toolbox = DockWindow_Create(Toolbox_Create(DockHost_GetHWND(host)));
  DockWindow* welcome2 = DockWindow_Create(Welcome_Create(DockHost_GetHWND(host), L"Welcome 2"));
  DockWindow* palette = DockWindow_Create(Palette_Create(DockHost_GetHWND(host)));
  DockWindow* optionbar = DockWindow_Create(OptionBar_Create(DockHost_GetHWND(host)));
  DockWindow* layers = DockWindow_Create(Layers_Create(DockHost_GetHWND(host)));

  DockContainer_Attach(split3, (DockBase*)toolbox);
  DockContainer_Attach(split3, (DockBase*)welcome2);

  DockContainer_Attach(split2, (DockBase*)palette);
  DockContainer_Attach(split2, (DockBase*)layers);

  DockContainer_Attach(split1, (DockBase*)split3);
  DockContainer_Attach(split1, (DockBase*)split2);

  DockContainer_Attach(root, (DockBase*)split1);
  DockContainer_Attach(root, (DockBase*)optionbar);

  hwndDockHost = DockHost_GetHWND(host);
}

Viewport* g_activeViewport;

Viewport* GetActiveViewport()
{
  return g_activeViewport; 
}

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_COMMAND:
      EndDialog(hwndDlg, wParam);
      return TRUE;
  }
  return FALSE;
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
      // init_open_file_dialog();
      Document_Open(Viewport_GetDocument(GetActiveViewport()));
      break;
    case IDM_FILE_SAVE:
      Document_Save(Viewport_GetDocument(GetActiveViewport()));
      break;
    case IDM_FILE_CLOSE:
      PostQuitMessage(0);
      break;
    case IDM_EDIT_TESTFILL:
      {
        Viewport* viewport = GetActiveViewport();
        Document* document = Viewport_GetDocument(viewport);
        Canvas* canvas = Document_GetCanvas(document);

        Canvas_FillSolid(canvas, 0xFFFFFFFF);
      }
      break;
    case IDM_EDIT_CLRCANVAS:
      {
        Viewport* viewport = GetActiveViewport();
        Document* document = Viewport_GetDocument(viewport);
        Canvas* canvas = Document_GetCanvas(document);

        Canvas_Clear(canvas);
      }
      break;
    case IDM_WINDOW_TOOLS:
      CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2),
                    IDM_WINDOW_TOOLS,
                    IsWindowVisible(hwndToolShelf) ? MF_UNCHECKED : MF_CHECKED);
      ShowWindow(hwndToolShelf,
                 IsWindowVisible(hwndToolShelf) ? SW_HIDE : SW_SHOW);
      break;
    case IDM_OPTIONS_SETTINGS:
      ShowSettingsWindow(hWnd);
      break;
    case IDM_HELP_ABOUT:
      DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlgProc);
      break;
    default:
      break;
    }
    break;
  case WM_SIZE:
  {
    WORD width = LOWORD(lParam);
    WORD height = HIWORD(lParam);

    SetWindowPos(hwndDockHost, NULL, 0, 0, width, height, SWP_NOACTIVATE |
        SWP_NOZORDER);
  } break;
  case WM_CREATE:
    Panitent_DockHostInit(hWnd);
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
  UnregisterClass(szViewportWndClass, NULL);
}

Document* GetActiveDocument()
{
  return g_activeDocument;
}

void SetActiveDocument(Document* document)
{
  g_activeDocument = document;
}

HWND Panitent_GetHWND()
{
  return g_panitent.hwnd_main;
}

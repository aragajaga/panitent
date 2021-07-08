#include "precomp.h"

#include <commctrl.h>

#include "panitent.h"

#include "wu_primitives.h"
#include "option_bar.h"
#include "file_open.h"
#include "bresenham.h"
#include "toolbox.h"
#include "dockhost.h"
#include "panitent.h"
#include "resource.h"
#include "settings.h"
#include "swatch2.h"
#include "viewport.h"
#include "palette.h"
#include "winuser.h"
#include "history.h"
#include "new.h"
#include "color_context.h"
#include "brush.h"
#include <assert.h>
#include <strsafe.h>
#include <stdbool.h>

#ifdef HAS_DISCORDSDK
#include "discordsdk.h"
#endif /* HAS_DISCORDSDK */

static HINSTANCE hInstance;
static HWND hwndToolShelf;

panitent_t g_panitent;

Viewport* g_activeViewport;

HFONT hFontSys;

const WCHAR szAppName[] = L"Panit.ent";

Document* Panitent_GetActiveDocument()
{
  Viewport* viewport = Panitent_GetActiveViewport();
  return viewport->document;
}

void Panitent_SetActiveViewport(Viewport* viewport)
{
  g_activeViewport = viewport;
}

Viewport* Panitent_GetActiveViewport()
{
  return g_activeViewport;
}

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

#ifdef HAS_DISCORDSDK
  g_panitent.discord = DiscordSDKInit();
  Discord_SetActivityStatus(g_panitent.discord, L"Idle");
#endif /* HAS_DISCORDSDK */

  hInstance = hInst;

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_TAB_CLASSES;
  InitCommonControlsEx(&icex);

  FetchSystemFont();

  InitColorContext();
  Viewport_RegisterClass(hInstance);
  DockHost_Register(hInstance);
  Toolbox_RegisterClass();
  PaletteWindow_RegisterClass(hInstance);
  OptionBar_RegisterClass(hInstance);
  SettingsWindow_Register(hInstance);
  SwatchControl2_RegisterClass(hInstance);
  BrushSel_RegisterClass(hInstance);

  bresenham_init();
  wu_init();
  g_primitives_context = g_bresenham_primitives;
  g_primitives_context.fStroke = TRUE;
  g_primitives_context.fFill = TRUE;

  InitializeBrushList();
  g_pBrush = &g_brushList[0];
  g_brushSize = 24;

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
  return (int)msg.wParam;
}

BOOL bConsoleAttached;

HWND hPaletteToolbox;
HWND hwndDockHost;

HWND hwndToolbox;
HWND hwndOptionBar;
HWND hwndPalette;

binary_tree_t* viewportNode;

Toolbox g_toolbox;

void Panitent_DockHostInit(HWND hWnd, binary_tree_t* parent)
{
  parent->posFixedGrip = 48;
  parent->gripAlign    = GRIP_ALIGN_END;
  parent->gripPosType  = GRIP_POS_ABSOLUTE;
  parent->bShowCaption = FALSE;
  parent->splitDirection = SPLIT_DIRECTION_VERTICAL;

  binary_tree_t* nodeOptionBar = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodeZ = calloc(1, sizeof(binary_tree_t));

  hwndOptionBar = CreateWindowEx(WS_EX_TOOLWINDOW,
                               L"Win32Class_OptionBar",
                               L"Option Bar",
                               WS_VISIBLE | WS_CHILD,
                               0,
                               32,
                               64,
                               256,
                               hWnd,
                               NULL,
                               GetModuleHandle(NULL),
                               NULL);
  assert(hwndOptionBar);

#ifdef DEBUG
  if (!hwndOptionBar)
  {
    OutputDebugString(L"OptionBar creation failed. "
        "Maybe class not registered?");
  }
#endif /* DEBUG */

  nodeOptionBar->lpszCaption  = L"Option Bar";
  nodeOptionBar->bShowCaption = TRUE;
  nodeOptionBar->hwnd         = hwndOptionBar;

  nodeZ->posFixedGrip = 128;
  nodeZ->gripAlign = GRIP_ALIGN_END;
  nodeZ->gripPosType = GRIP_POS_ABSOLUTE;
  nodeZ->bShowCaption = FALSE;

  /* Working Area */
  binary_tree_t* nodeY = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodePalette = calloc(1, sizeof(binary_tree_t));

  nodeY->posFixedGrip = 64;
  nodeY->gripAlign    = GRIP_ALIGN_START;
  nodeY->gripPosType  = GRIP_POS_ABSOLUTE;

  hwndPalette = CreateWindowEx(0,
                               L"Win32Class_PaletteWindow",
                               L"Palette",
                               WS_CHILD | WS_VISIBLE,
                               0,
                               0,
                               128,
                               128,
                               hWnd,
                               NULL,
                               GetModuleHandle(NULL),
                               NULL);

  nodePalette->lpszCaption  = L"Palette";
  nodePalette->bShowCaption = TRUE;
  nodePalette->hwnd         = hwndPalette;

  /* Toolbox and viewport split */
  binary_tree_t* nodeToolbox = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodeViewport = calloc(1, sizeof(binary_tree_t));

  hwndToolbox = CreateWindowEx(WS_EX_TOOLWINDOW,
                               TOOLBOX_WC,
                               L"Tools",
                               WS_VISIBLE | WS_CHILD,
                               0,
                               32,
                               64,
                               256,
                               hWnd,
                               NULL,
                               GetModuleHandle(NULL),
                               (LPVOID)&g_toolbox);

  nodeToolbox->lpszCaption  = L"Tool";
  nodeToolbox->bShowCaption = TRUE;
  nodeToolbox->hwnd         = hwndToolbox;

  nodeViewport->lpszCaption  = L"Viewport";
  nodeViewport->bShowCaption = TRUE;
  viewportNode         = nodeViewport;

  /* Set graph */
  nodeY->node1 = nodeToolbox;
  nodeY->node2 = nodeViewport;

  nodeZ->node1 = nodeY;
  nodeZ->node2 = nodePalette;

  parent->node1 = nodeZ;
  parent->node2 = nodeOptionBar;
}

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
#ifdef HAS_DISCORDSDK
      Discord_SetActivityStatus(g_panitent.discord, L"Seeking About dialog 👀");
#endif /* HAS_DISCORDSDK */
      return TRUE;
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
      Document_Open(Panitent_GetActiveViewport()->document);
      break;
    case IDM_FILE_SAVE:
      Document_Save(Panitent_GetActiveViewport()->document);
      break;
    case IDM_FILE_CLOSE:
      PostQuitMessage(0);
      break;
    case IDM_EDIT_UNDO:
      History_Undo(Panitent_GetActiveDocument());
      break;
    case IDM_EDIT_REDO:
      History_Redo(Panitent_GetActiveDocument());
      break;
    case IDM_EDIT_CLRCANVAS:
      Canvas_Clear(Panitent_GetActiveViewport()->document->canvas);
      Viewport_Invalidate(Panitent_GetActiveViewport());
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
    default:
      break;
    }
    break;
  case WM_SIZE:
  {
    WORD cx = LOWORD(lParam);
    WORD cy = HIWORD(lParam);

    SetWindowPos(hwndDockHost, NULL, 0, 0, cx, cy,
        SWP_NOACTIVATE | SWP_NOZORDER);

  } break;

  case WM_CREATE:
    hwndDockHost = DockHost_Create(hWnd);

    RECT rcDockHost = {0};
    GetClientRect(hwndDockHost, &rcDockHost);
    root              = calloc(1, sizeof(binary_tree_t));
    root->lpszCaption = L"Root";
    root->rc          = rcDockHost;

    Panitent_DockHostInit(hwndDockHost, root);

    /* option_bar_create(hWnd); */

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

  hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
      SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hParent,
      (HMENU)IDS_STATUS, GetModuleHandle(NULL), NULL);

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
  AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_UNDO, L"&Undo");
  AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_REDO, L"&Redo");
  AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_CLRCANVAS, L"&Clear canvas");
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
  Toolbox_UnregisterClass();
}

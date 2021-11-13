#include "precomp.h"

#include "panitent.h"

#include "bresenham.h"
#include "brush.h"
#include "canvas.h"
#include "color_context.h"
#include "dockhost.h"
#include "history.h"
#include "new.h"
#include "option_bar.h"
#include "palette.h"
#include "primitives_context.h"
#include "settings.h"
#include "swatch2.h"
#include "toolbox.h"
#include "viewport.h"
#include "wu_primitives.h"
#include "settings.h"
#include "pentablet.h"

#include "resource.h"

static HINSTANCE g_hInstance;

Panitent g_panitent;

HFONT hFontSys;
BOOL PanitentWindow_RegisterClass(HINSTANCE hInstance);
INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam);
LRESULT CALLBACK PanitentWindow_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam);

HWND hPaletteToolbox;
HWND hwndDockHost;

HWND hwndToolbox;
HWND hwndOptionBar;
HWND hwndPalette;

void Panitent_DockHostInit(HWND hwndDockHost, binary_tree_t* root);

binary_tree_t* viewportNode;

Toolbox g_toolbox;

const WCHAR szAppName[] = L"Panit.ent";
const WCHAR szPanitentWndClass[] = L"Win32Class_PanitentWindow";

void Panitent_RegisterClasses(HINSTANCE hInstance);
HMENU CreateMainMenu();
void FetchSystemFont();
BOOL PanitentWindow_RegisterClass(HINSTANCE hInstance);

/* Menu ID values */
enum {
  IDM_FILE_NEW = 1001,
  IDM_FILE_OPEN,
  IDM_FILE_SAVE,
  IDM_FILE_CLOSE,

  IDM_EDIT_UNDO,
  IDM_EDIT_REDO,
  IDM_EDIT_CLRCANVAS,

  IDM_WINDOW_TOOLS,

  IDM_OPTIONS_SETTINGS,

  IDM_HELP_TOPICS,
  IDM_HELP_ABOUT
};

/* Entry point for application */
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance)
  UNREFERENCED_PARAMETER(lpCmdLine)

#ifdef HAS_DISCORDSDK
  /* Initialize Discord SDK and set initial activity message */

  g_panitent.discord = DiscordSDKInit();
  Discord_SetActivityStatus(g_panitent.discord, L"Idle");
#endif /* HAS_DISCORDSDK */

  /* Share application instance */
  g_panitent.hInstance = hInstance;

  /* Request Common Controls v6 */
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_TAB_CLASSES;
  InitCommonControlsEx(&icex);

  FetchSystemFont();
  InitColorContext();

  /* Register custom controls and windows classes */
  Panitent_RegisterClasses(hInstance);

  bresenham_init();
  wu_init();
  g_primitives_context = g_bresenham_primitives;
  g_primitives_context.fStroke = TRUE;
  g_primitives_context.fFill = TRUE;

  InitializeBrushList();
  g_pBrush = &g_brushList[0];
  g_brushSize = 24;

  HMENU hMenu = CreateMainMenu();

  /* Regiser application window class and create it */
  PanitentWindow_RegisterClass(hInstance);

  PNTSETTINGS *pSettings;
  pSettings = Panitent_GetSettings();

  FILE *fp;
  fp = fopen("settings.dat", "rb");
  assert(fp);
  fread(pSettings, sizeof(PNTSETTINGS), 1, fp);
  fclose(fp);

  DWORD windowX = CW_USEDEFAULT;
  DWORD windowY = 0;
  DWORD windowWidth = CW_USEDEFAULT;
  DWORD windowHeight = 0;

  if (pSettings->bRememberWindowPos)
  {
    windowX = pSettings->x;
    windowY = pSettings->y;
    windowWidth = pSettings->width;
    windowHeight = pSettings->height;
  }

  HWND hwnd = CreateWindowEx(0, szPanitentWndClass, szAppName,
      WS_OVERLAPPEDWINDOW,
      windowX, windowY, windowWidth, windowHeight,
      NULL, NULL, hInstance, NULL);

  g_panitent.hwnd = hwnd;

  ShowWindow(hwnd, nCmdShow);
  SetMenu(hwnd, hMenu);

  /* Initialize Wintab API */
  if (pSettings->bEnablePenTablet)
    LoadWintab();

  /* Application message loop.
   * Just pump inbound window messages and forward them to associated with
   * window procedure */
  MSG msg;
  ZeroMemory(&msg, sizeof(MSG));
  while (GetMessage(&msg, NULL, 0, 0)) {
    DispatchMessage(&msg);
    TranslateMessage(&msg);
  }

  return (int)msg.wParam;
}

void Panitent_OnClose(HWND hWnd)
{
  PNTSETTINGS* pSettings = NULL;
  RECT rc = {0};

  pSettings = Panitent_GetSettings();
  assert(pSettings);

  GetWindowRect(hWnd, &rc);

  if (pSettings->bRememberWindowPos)
  {
    if (pSettings->x == (DWORD)CW_USEDEFAULT ||
        pSettings->y == (DWORD)CW_USEDEFAULT)
    {
      pSettings->x = CW_USEDEFAULT;
      pSettings->y = 0;
    }
    else {
      pSettings->x = rc.left;
      pSettings->y = rc.top;
    }

    if (pSettings->width == (DWORD)CW_USEDEFAULT ||
        pSettings->height == (DWORD)CW_USEDEFAULT)
    {
      pSettings->width = CW_USEDEFAULT;
      pSettings->height = 0;
    }
    else {
      pSettings->width = rc.right - rc.left;
      pSettings->height= rc.bottom - rc.top;
    }
  }

  FILE *fp = NULL;
  fp = fopen("settings.dat", "wb");
  fwrite(pSettings, sizeof(PNTSETTINGS), 1, fp);
  fclose(fp);

  PostQuitMessage(0);
}

void Panitent_OnCreate(HWND hWnd) {

  hwndDockHost = DockHost_Create(hWnd);

  RECT rcDockHost = {0};
  GetClientRect(hwndDockHost, &rcDockHost);
  root = calloc(1, sizeof(binary_tree_t));
  root->lpszCaption = L"Root";
  root->rc = rcDockHost;

  Panitent_DockHostInit(hwndDockHost, root);
}

/* Main window message handling procedure */
LRESULT CALLBACK PanitentWindow_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      Panitent_OnCreate(hWnd);
      return 0;

    case WM_SIZE:
      {
        WORD width = LOWORD(lParam);
        WORD height = HIWORD(lParam);

        /* Adjust dock host control to new window size */
        SetWindowPos(hwndDockHost, NULL, 0, 0, width, height,
            SWP_NOACTIVATE | SWP_NOZORDER);
      }
      return 0;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDM_FILE_NEW:
          NewFileDialog(hWnd);
          break;

        case IDM_FILE_OPEN:
          Document_Open(NULL);
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
          CheckMenuItem(GetSubMenu(GetMenu(hWnd), 2), IDM_WINDOW_TOOLS,
              IsWindowVisible(hwndToolbox) ? MF_UNCHECKED : MF_CHECKED);
          ShowWindow(hwndToolbox,
              IsWindowVisible(hwndToolbox) ? SW_HIDE : SW_SHOW);
          break;

        case IDM_OPTIONS_SETTINGS:
          ShowSettingsWindow(hWnd);
          break;

        case IDM_HELP_ABOUT:
          DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlgProc);
          break;
        }
      return 0;

    case WM_THEMECHANGED:
      FetchSystemFont();
      return 0;

    case WM_DESTROY:
      Panitent_OnClose(hWnd);
      PostQuitMessage(0);
      return 0;

    }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

/* Main window control class registerer */
BOOL PanitentWindow_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)PanitentWindow_WndProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = szPanitentWndClass;

  return RegisterClassEx(&wcex);
}

void Panitent_RegisterClasses(HINSTANCE hInstance)
{
  BOOL bStatus;

  Viewport_RegisterClass(hInstance);
  DockHost_Register(hInstance);
  Toolbox_RegisterClass();
  PaletteWindow_RegisterClass(hInstance);
  OptionBar_RegisterClass(hInstance);
  SettingsWindow_Register(hInstance);
  SwatchControl2_RegisterClass(hInstance);
  BrushSel_RegisterClass(hInstance);
}

void Panitent_DockHostInit(HWND hWnd, binary_tree_t* parent)
{
  parent->posFixedGrip = 48;
  parent->gripAlign    = GRIP_ALIGN_END;
  parent->gripPosType  = GRIP_POS_ABSOLUTE;
  parent->bShowCaption = FALSE;
  parent->splitDirection = SPLIT_DIRECTION_VERTICAL;

  binary_tree_t* nodeOptionBar = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodeZ = calloc(1, sizeof(binary_tree_t));

  hwndOptionBar = CreateWindowEx(WS_EX_TOOLWINDOW, L"Win32Class_OptionBar",
      L"Option Bar",
      WS_VISIBLE | WS_CHILD,
      0, 32, 64, 256,
      hWnd, NULL, GetModuleHandle(NULL), NULL);

  assert(hwndOptionBar);

#ifndef NDEBUG
  if (!hwndOptionBar)
  {
    OutputDebugString(
        L"OptionBar creation failed. Maybe class not registered?");
  }
#endif /* NDEBUG */

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

  hwndPalette = CreateWindowEx(0, L"Win32Class_PaletteWindow", L"Palette",
      WS_CHILD | WS_VISIBLE,
      0, 0, 128, 128,
      hWnd, NULL, GetModuleHandle(NULL), NULL);

  nodePalette->lpszCaption  = L"Palette";
  nodePalette->bShowCaption = TRUE;
  nodePalette->hwnd         = hwndPalette;

  /* Toolbox and viewport split */
  binary_tree_t* nodeToolbox = calloc(1, sizeof(binary_tree_t));
  binary_tree_t* nodeViewport = calloc(1, sizeof(binary_tree_t));

  hwndToolbox = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBOX_WC, L"Tools",
      WS_VISIBLE | WS_CHILD,
      0, 32, 64, 256,
      hWnd, NULL, GetModuleHandle(NULL), (LPVOID)&g_toolbox);

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

Document* Panitent_GetActiveDocument()
{
  Viewport* viewport = Panitent_GetActiveViewport();
  return viewport->document;
}

void Panitent_SetActiveViewport(Viewport* viewport)
{
  g_panitent.activeViewport = viewport;
}

Viewport* Panitent_GetActiveViewport()
{
  return g_panitent.activeViewport;
}

HWND Panitent_GetHWND()
{
  return g_panitent.hwnd;
}

PNTSETTINGS *Panitent_GetSettings()
{
  return &g_panitent.settings;
}

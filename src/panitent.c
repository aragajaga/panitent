#include "precomp.h"

#include "panitent.h"

#include "win32/application.h"

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
#include "settings_dialog.h"
#include "log_window.h"
#include "log.h"
#include "palette.h"
#include "palette_window.h"

#include "resource.h"

#include <DbgHelp.h>

DOCKHOSTDATA g_dockHost;

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

BOOL Panitent_RegisterClasses(HINSTANCE hInstance);
HMENU CreateMainMenu();
void FetchSystemFont();
BOOL PanitentWindow_RegisterClass(HINSTANCE hInstance);

/* Menu ID values */
enum {
  IDM_FILE_NEW = 1001,
  IDM_FILE_OPEN,
  IDM_FILE_SAVE,
  IDM_FILE_CLIPBOARD_EXPORT,
  IDM_FILE_CLOSE,

  IDM_EDIT_UNDO,
  IDM_EDIT_REDO,
  IDM_EDIT_CLRCANVAS,

  IDM_WINDOW_TOOLS,

  IDM_OPTIONS_SETTINGS,

  IDM_HELP_TOPICS,
  IDM_HELP_LOG,
  IDM_HELP_ABOUT
};

void CreateDump(EXCEPTION_POINTERS* exceptionPointers)
{
  SYSTEMTIME st;
  GetSystemTime(&st);

  WCHAR szDumpFileName[MAX_PATH];
  StringCchPrintf(szDumpFileName, MAX_PATH, L"panitent_%04d%02d%02d_%02d%02d%02d.dmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);


  HANDLE dumpFile = CreateFile(szDumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (dumpFile != INVALID_HANDLE_VALUE)
  {
    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = { 0 };
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = exceptionPointers;
    exceptionInfo.ClientPointers = TRUE;

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpWithFullMemory, &exceptionInfo, NULL, NULL);
    CloseHandle(dumpFile);
  }
}

LONG WINAPI PanitentUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
  CreateDump(exceptionPointers);
  return EXCEPTION_EXECUTE_HANDLER;
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 28251 )
#endif  /* _MSC_VER */

/* Entry point for application */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPWSTR lpCmdLine, int nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);

  BOOL bResult;
  LPWSTR *pszArgList;
  int nArgs;
  LPWSTR pszArgFile;
  WCHAR szModulePath[MAX_PATH];
  WCHAR szWorkDir[MAX_PATH];
  WCHAR szAppData[MAX_PATH];
  MSG msg;

  struct PanitentApplication* app = PanitentApplication_Create();

  Window_CreateWindow((struct Window*)app->paletteWindow, NULL);

  AddVectoredExceptionHandler(TRUE, PanitentUnhandledExceptionFilter);

  pszArgFile = NULL;

  /*  Get command line arguments
   *
   *  If lpCmdLine is empty, the CommandLineToArgvW will return the
   *  executable path
   */
  if (lpCmdLine && *lpCmdLine != L'\0')
  {
    pszArgList = CommandLineToArgvW(lpCmdLine, &nArgs);

    if (nArgs > 0)
      pszArgFile = pszArgList[0];
  }

  ZeroMemory(szModulePath, sizeof(szModulePath));
  GetModuleFileName(GetModuleHandle(NULL), szModulePath, MAX_PATH);

  ZeroMemory(szWorkDir, sizeof(szWorkDir));
  GetCurrentDirectory(MAX_PATH, szWorkDir);

  PWSTR lpszAppData;
  ZeroMemory(szAppData, sizeof(szAppData));
  SHGetKnownFolderPath(&FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &lpszAppData);
  StringCchCopy(szAppData, MAX_PATH, lpszAppData);
  CoTaskMemFree(lpszAppData);
  PathAppend(szAppData, L"\\Aragajaga\\Panit.ent");

  /*  Create %APPDATA% application folder
   *
   *  This function is available through XP SP2 and Server 2003.
   *  It might be altered or unavailable in subsequent versions of OS.
   */
  SHCreateDirectoryEx(NULL, szAppData, NULL);

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
  icex.dwICC = ICC_TAB_CLASSES;
  InitCommonControlsEx(&icex);

  FetchSystemFont();
  InitColorContext();

  /* Register custom controls and windows classes */
  bResult = Panitent_RegisterClasses(hInstance);
  assert(bResult);
  if (!bResult)
  {
    return -1;
  }

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

  PNTSETTINGS* pSettings;
  pSettings = Panitent_GetSettings();

  WCHAR szSettingsPath[MAX_PATH];
  StringCchCopy(szSettingsPath, MAX_PATH, szAppData);
  PathAppend(szSettingsPath, L"\\settings.dat");

  FILE* fp;
  errno_t result = _wfopen_s(&fp, szSettingsPath, L"rb");
  assert(result);
  if (result && fp)
  {
    fread(pSettings, sizeof(PNTSETTINGS), 1, fp);
    fclose(fp);
  }

  int windowX = CW_USEDEFAULT;
  int windowY = 0;
  int windowWidth = CW_USEDEFAULT;
  int windowHeight = 0;

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
  /*
  if (pSettings->bEnablePenTablet)
    LoadWintab();
    */

  if (pszArgFile)
  {
    Panitent_OpenFile(pszArgFile);
  }

  /* Application message loop.
   * Just pump inbound window messages and forward them to associated window
     procedure */
  ZeroMemory(&msg, sizeof(MSG));
  while (GetMessage(&msg, NULL, 0, 0)) {
    DispatchMessage(&msg);
    TranslateMessage(&msg);
  }

  return (int)msg.wParam;
}

#ifdef _MSCVER
#pragma warning( pop )
#endif  /* _MSC_VER */

void Panitent_OnClose(HWND hWnd)
{
  PNTSETTINGS* pSettings = NULL;
  RECT rc = { 0 };

  pSettings = Panitent_GetSettings();
  assert(pSettings);

  GetWindowRect(hWnd, &rc);

  if (pSettings->bRememberWindowPos)
  {
    if (pSettings->x == CW_USEDEFAULT ||
      pSettings->y == CW_USEDEFAULT)
    {
      pSettings->x = CW_USEDEFAULT;
      pSettings->y = 0;
    }
    else {
      pSettings->x = rc.left;
      pSettings->y = rc.top;
    }

    if (pSettings->width == CW_USEDEFAULT ||
      pSettings->height == CW_USEDEFAULT)
    {
      pSettings->width = CW_USEDEFAULT;
      pSettings->height = 0;
    }
    else {
      pSettings->width = rc.right - rc.left;
      pSettings->height = rc.bottom - rc.top;
    }
  }

  FILE* fp = NULL;
  errno_t result = _wfopen_s(&fp, L"settings.dat", L"wb");
  assert(result == 0 && fp);
  if (result == 0 && fp)
  {
    fwrite(pSettings, sizeof(PNTSETTINGS), 1, fp);
    fclose(fp);
  }

  PostQuitMessage(0);
}

void Panitent_OnCreate(HWND hWnd) {

  DockHost_Init(&g_dockHost);
  DockHost_Create(&g_dockHost, hWnd);

  RECT rcDockHost = {0};
  GetClientRect(g_dockHost.hWnd_, &rcDockHost);
  g_dockHost.pRoot_ = calloc(1, sizeof(binary_tree_t));
  if (!g_dockHost.pRoot_)
    return;

  g_dockHost.pRoot_->lpszCaption = L"Root";
  g_dockHost.pRoot_->rc = rcDockHost;

  Panitent_DockHostInit(g_dockHost.hWnd_, g_dockHost.pRoot_);
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
        SetWindowPos(g_dockHost.hWnd_, NULL, 0, 0, width, height,
            SWP_NOACTIVATE | SWP_NOZORDER);
      }
      return 0;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDM_FILE_NEW:
          LogMessage(LOGENTRY_TYPE_INFO, L"MAIN", L"New File menu issued");
          NewFileDialog(hWnd);
          break;

        case IDM_FILE_OPEN:
          Panitent_Open();
          break;

        case IDM_FILE_SAVE:
          Document_Save(Panitent_GetActiveViewport()->document);
          break;

        case IDM_FILE_CLIPBOARD_EXPORT:
          Panitent_ClipboardExport();
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

        case IDM_HELP_LOG:
          LogWindow_Create(hWnd);
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

static inline void PopupClassRegistrationFail(LPWSTR lpszTip)
{
  WCHAR szMessage[256];
  ZeroMemory(szMessage, sizeof(szMessage));

  StringCchPrintf(szMessage, 256, L"Failed to register %s class", lpszTip);

  MessageBox(NULL, szMessage, NULL, MB_OK | MB_ICONERROR);
}

static inline BOOL AssertClassRegistration(
    HINSTANCE hInstance,
    LPWSTR lpszClassName,
    BOOL (*lpfnClassRegisterer)(HINSTANCE))
{
  BOOL bResult;

  bResult = (*lpfnClassRegisterer)(hInstance);
  assert(bResult);
  if (!bResult)
  {
    PopupClassRegistrationFail(lpszClassName);
  }

  return TRUE;
}

BOOL Panitent_RegisterClasses(HINSTANCE hInstance)
{
  BOOL bStatus;

  bStatus = TRUE;

  bStatus = bStatus && AssertClassRegistration( hInstance, L"SettingsDialog",
      SettingsDialog_RegisterClass);

  bStatus = bStatus && AssertClassRegistration(hInstance, L"Viewport",
      Viewport_RegisterClass);

  /*
  bStatus = bStatus && AssertClassRegistration(hInstance, L"DockHost",
      DockHost_RegisterClass);
      */

  bStatus = bStatus && AssertClassRegistration(hInstance, L"ToolBox",
      Toolbox_RegisterClass);

  bStatus = bStatus && AssertClassRegistration(hInstance, L"OptonBar",
      OptionBar_RegisterClass);

  bStatus = bStatus && AssertClassRegistration(hInstance, L"SettingsWindow",
      SettingsWindow_Register);

  bStatus = bStatus && AssertClassRegistration(hInstance, L"SwatchControl2",
      SwatchControl2_RegisterClass);

  bStatus = bStatus && AssertClassRegistration(hInstance, L"BrushSel",
      BrushSel_RegisterClass);

  bStatus = bStatus && AssertClassRegistration(hInstance, L"Logwindow",
      LogWindow_Register);

  return bStatus;
}

void Panitent_DockHostInit(HWND hWnd, binary_tree_t* parent)
{
  parent->posFixedGrip = 48;
  parent->gripAlign    = GRIP_ALIGN_END;
  parent->gripPosType  = GRIP_POS_ABSOLUTE;
  parent->bShowCaption = FALSE;
  parent->splitDirection = SPLIT_DIRECTION_VERTICAL;

  binary_tree_t* nodeOptionBar = calloc(1, sizeof(binary_tree_t));
  if (!nodeOptionBar)
  {
    return;
  }

  binary_tree_t* nodeZ = calloc(1, sizeof(binary_tree_t));
  if (!nodeZ)
  {
    free(nodeOptionBar);
    return;
  }

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
  if (!nodeY)
    return;

  binary_tree_t* nodePalette = calloc(1, sizeof(binary_tree_t));
  if (!nodePalette)
    return;

  nodeY->posFixedGrip = 64;
  nodeY->gripAlign    = GRIP_ALIGN_START;
  nodeY->gripPosType  = GRIP_POS_ABSOLUTE;

  hwndPalette = CreateWindowEx(0, L"Win32Class_PaletteWindow", L"Palette",
      WS_CHILD | WS_VISIBLE,
      0, 0, 128, 128,
      hWnd, NULL, GetModuleHandle(NULL), NULL);

  hwndPalette = NULL;

  nodePalette->lpszCaption  = L"Palette";
  nodePalette->bShowCaption = TRUE;
  nodePalette->hwnd         = hwndPalette;

  /* Toolbox and viewport split */
  binary_tree_t* nodeToolbox = calloc(1, sizeof(binary_tree_t));
  if (!nodeToolbox)
    return;

  binary_tree_t* nodeViewport = calloc(1, sizeof(binary_tree_t));
  if (!nodeViewport)
    return;

  hwndToolbox = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBOX_WC, L"Tools",
      WS_VISIBLE | WS_CHILD,
      0, 32, 128, 256,
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
  UNREFERENCED_PARAMETER(lParam);

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
  AppendMenu(hSubMenu, MF_STRING, IDM_FILE_CLIPBOARD_EXPORT, L"Export image to clipboard");
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
  AppendMenu(hSubMenu, MF_STRING, IDM_HELP_LOG, L"&Log");
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

void Panitent_Open()
{
  Document_Open(NULL);
}

void Panitent_OpenFile(LPWSTR pszPath)
{
  Document_OpenFile(pszPath);
}

Viewport* Panitent_CreateViewport()
{
  Viewport* pViewport = Panitent_GetActiveViewport();

  if (!pViewport)
  {
    HWND hWndViewport;

    hWndViewport = CreateWindowEx(0, WC_VIEWPORT, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        64, 0, 800, 600,
        g_panitent.hwnd,
        NULL, GetModuleHandle(NULL), NULL);

    assert(hWndViewport);

    pViewport = (Viewport *) GetWindowLongPtr(hWndViewport, GWLP_USERDATA);
    Panitent_SetActiveViewport(pViewport);

    viewportNode->hwnd = hWndViewport;
    DockNode_arrange(g_dockHost.pRoot_);
  }

  return pViewport;
}

void Panitent_ClipboardExport() {
  HWND hWndPanitent;
  /* HGLOBAL hglbCopy; */

  hWndPanitent = Panitent_GetHWND();

  if (!OpenClipboard(hWndPanitent)) {
    return;
  }
  EmptyClipboard();

  Document *doc = Panitent_GetActiveDocument();
  if (!doc)
    return;

  Canvas *canvas = Document_GetCanvas(doc);
  if (!canvas)
    return;

  unsigned char *pData = malloc(canvas->buffer_size);
  if (!pData)
    return;

  ZeroMemory(pData, canvas->buffer_size);
  memcpy(pData, canvas->buffer, canvas->buffer_size);

  for (size_t y = 0; y < (size_t) canvas->height; y++) {
    for (size_t x = 0; x < (size_t) canvas->width; x++) {
      uint8_t *pixel = pData + (x + y * canvas->width) * 4;

      float factor = (float)(pixel[3]) / 255.f;

      pixel[0] *= (uint8_t)factor;
      pixel[1] *= (uint8_t)factor;
      pixel[2] *= (uint8_t)factor;
    }
  }

  HBITMAP hBitmap = CreateBitmap(canvas->width,
      canvas->height, 1, sizeof(uint32_t) * 8, pData);

  /*
  hglbCopy = GlobalAlloc(GMEM_MOVEABLE, sizeof(HBITMAP));
  if (!hglbCopy) {
    CloseClipboard();
    return;
  }
  */

  SetClipboardData(CF_BITMAP, hBitmap);
  CloseClipboard();
}

struct PanitentApplication* PanitentApplication_Create()
{
  struct PanitentApplication* app = calloc(1, sizeof(struct PanitentApplication));

  if (app)
  {
    PanitentApplication_Init(app);
  }

  return app;
}

void PanitentApplication_Init(struct PanitentApplication* app)
{
  Application_Init(&app->base);

  app->palette = Palette_Create();
  app->paletteWindow = PaletteWindow_Create(app, app->palette);
}

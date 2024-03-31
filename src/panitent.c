#include "precomp.h"

#include "win32/application.h"
#include "win32/window.h"
#include "win32/framework.h"

#include "palette.h"
#include "panitent.h"

#include "document.h"
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
#include "glwindow.h"
#include "panitentwindow.h"
#include "crashhandler.h"
#include "workspacecontainer.h"
#include "tree.h"
#include "layerswindow.h"

#include "resource.h"

static HINSTANCE g_hInstance;

Panitent g_panitent;

HFONT hFontSys;
INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
    LPARAM lParam);

HWND hPaletteToolbox;
HWND hwndDockHost;

HWND hwndToolbox;
HWND hwndOptionBar;
HWND hwndPalette;

TreeNode* viewportNode;

const WCHAR szAppName[] = L"Panit.ent";
const WCHAR szPanitentWndClass[] = L"Win32Class_PanitentWindow";

BOOL Panitent_RegisterClasses(HINSTANCE hInstance);
HMENU CreateMainMenu();
void FetchSystemFont();

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

struct PanitentApplication* g_app;

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

  /* Request Common Controls v6 */
  INITCOMMONCONTROLSEX icex = { 0 };
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icex);

  struct PanitentApplication* app = PanitentApplication_Create();
  g_app = app;

  AddVectoredExceptionHandler(TRUE, PanitentUnhandledExceptionFilter);

  WindowingInit();

  // Window_CreateWindow((struct Window*)app->m_pLayeredWindow, NULL);

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

  HWND hWnd = Window_CreateWindow((struct Window*)app->m_pPanitentWindow, NULL);

  g_panitent.hwnd = hWnd;

  ShowWindow(hWnd, nCmdShow);
  SetMenu(hWnd, hMenu);

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

  /*
  bStatus = bStatus && AssertClassRegistration(hInstance, L"DockHost",
      DockHost_RegisterClass);
      */

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

void Panitent_DockHostInit(DockHostWindow* pDockHostWindow, TreeNode* pNodeParent)
{

    DockData* pDockDataParent = (DockData*)calloc(1, sizeof(DockData));
    pNodeParent->data = (void*)pDockDataParent;
    pDockDataParent->dwStyle = DGA_END | DGP_ABSOLUTE | DGD_VERTICAL;
    pDockDataParent->iGripPos = 64;
    pDockDataParent->bShowCaption = FALSE;
    wcscpy_s(pDockDataParent->lpszName, MAX_PATH, L"Root");

    TreeNode* pNodeOptionBar = BinaryTree_AllocEmptyNode();
    DockData* pDockDataOptionBar = (DockData*)calloc(1, sizeof(DockData));
    pNodeOptionBar->data = (void*)pDockDataOptionBar;
    wcscpy_s(pDockDataOptionBar->lpszName, MAX_PATH, L"OptionBar");
    OptionBarWindow* pOptionBarWindow = OptionBarWindow_Create((Application*)g_app);
    Window_CreateWindow((Window*)pOptionBarWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataOptionBar, (Window*)pOptionBarWindow);

    /* Main node */
    TreeNode* pNodeZ = BinaryTree_AllocEmptyNode();
    DockData* pDockDataZ = (DockData*)calloc(1, sizeof(DockData));
    pNodeZ->data = (void*)pDockDataZ;
    pDockDataZ->dwStyle = DGA_END | DGP_ABSOLUTE | DGD_HORIZONTAL;
    pDockDataZ->iGripPos = 192;
    pDockDataZ->bShowCaption = FALSE;
    wcscpy_s(pDockDataZ->lpszName, MAX_PATH, L"Main");

    /* ======================================================================================== */

    /* Toolbox node */
    TreeNode* pNodeToolbox = BinaryTree_AllocEmptyNode();
    DockData* pDockDataToolbox = (DockData*)calloc(1, sizeof(DockData));
    pNodeToolbox->data = (void*)pDockDataToolbox;
    wcscpy_s(pDockDataToolbox->lpszName, MAX_PATH, L"Toolbox");
    ToolboxWindow* pToolboxWindow = ToolboxWindow_Create((struct Application*)g_app);
    hwndToolbox = Window_CreateWindow((Window*)pToolboxWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataToolbox, (Window*)pToolboxWindow);

    /* Viewport node */
    TreeNode* pNodeViewport = BinaryTree_AllocEmptyNode();
    DockData* pDockDataViewport = (DockData*)calloc(1, sizeof(DockData));
    pNodeViewport->data = (void*)pDockDataViewport;
    wcscpy_s(pDockDataViewport->lpszName, MAX_PATH, L"WorkspaceContainer");
    WorkspaceContainer* pWorkspaceContainer = WorkspaceContainer_Create((Application*)g_app);
    HWND hWndWorkspaceContainer = Window_CreateWindow((Window*)pWorkspaceContainer, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataViewport, (Window*)pWorkspaceContainer);
    g_app->m_pWorkspaceContainer = pWorkspaceContainer;

    /* Toolbox/Viewport node */
    TreeNode* pNodeY = BinaryTree_AllocEmptyNode();
    DockData* pDockDataY = (DockData*)calloc(1, sizeof(DockData));
    pNodeY->data = (void*)pDockDataY;
    wcscpy_s(pDockDataY->lpszName, MAX_PATH, L"Toolbox/Viewport");
    pDockDataY->dwStyle = DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL;
    pDockDataY->iGripPos = 86;

    /* Set graph */
    pNodeY->node1 = pNodeToolbox;
    pNodeY->node2 = pNodeViewport;

    /* ======================================================================================== */

    /* GLWindow node */
    TreeNode* pNodeGLWindow = BinaryTree_AllocEmptyNode();
    DockData* pDockDataGLWindow = (DockData*)calloc(1, sizeof(DockData));
    pNodeGLWindow->data = (void*)pDockDataGLWindow;
    wcscpy_s(pDockDataGLWindow->lpszName, MAX_PATH, L"GLWindow");
    GLWindow* pGLWindow = GLWindow_Create((struct Application*)g_app);
    HWND hwndGLWindow = Window_CreateWindow((Window*)pGLWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataGLWindow, (Window*)pGLWindow);

    /* Create and pin palette window */
    TreeNode* pNodePalette = BinaryTree_AllocEmptyNode();
    DockData* pDockDataPalette = (DockData*)calloc(1, sizeof(DockData));
    pNodePalette->data = (void*)pDockDataPalette;
    wcscpy_s(pDockDataPalette->lpszName, MAX_PATH, L"Palette");
    PaletteWindow* pPaletteWindow = PaletteWindow_Create((Application*)g_app, g_app->palette);
    hwndPalette = Window_CreateWindow((Window*)pPaletteWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataPalette, (Window*)pPaletteWindow);

    /* Create and pin layers window */
    TreeNode* pNodeLayers = BinaryTree_AllocEmptyNode();
    DockData* pDockDataLayers = (DockData*)calloc(1, sizeof(DockData));
    pNodeLayers->data = (void*)pDockDataLayers;
    wcscpy_s(pDockDataLayers->lpszName, MAX_PATH, L"Layers");
    LayersWindow* pLayersWindow = LayersWindow_Create((Application*)g_app);
    HWND hwndLayers = Window_CreateWindow((Window*)pLayersWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataLayers, (Window*)pLayersWindow);

    /* Palette/Layers node */
    TreeNode* pNodeB = BinaryTree_AllocEmptyNode();
    DockData* pDockDataB = (DockData*)calloc(1, sizeof(DockData));
    pNodeB->data = (void*)pDockDataB;
    wcscpy_s(pDockDataB->lpszName, MAX_PATH, L"Palette/Layers");
    pDockDataB->dwStyle = DGA_START | DGP_ABSOLUTE | DGD_VERTICAL;
    pDockDataB->iGripPos = 256;
    pDockDataB->bShowCaption = FALSE;

    /* Set graph */
    pNodeB->node1 = pNodePalette;
    pNodeB->node2 = pNodeLayers;



    /* right bar node */
    TreeNode* pNodeA = BinaryTree_AllocEmptyNode();
    DockData* pDockDataA = (DockData*)calloc(1, sizeof(DockData));
    pNodeA->data = (void*)pDockDataA;
    wcscpy_s(pDockDataA->lpszName, MAX_PATH, L"Right bar");
    pDockDataA->dwStyle = DGA_START | DGP_ABSOLUTE | DGD_VERTICAL;
    pDockDataA->iGripPos = 192;
    pDockDataA->bShowCaption = FALSE;

    /* Set graph */
    pNodeA->node1 = pNodeGLWindow;
    pNodeA->node2 = pNodeB;

    /* ======================================================================================== */

    /* Set  */
    pNodeZ->node1 = pNodeY;
    pNodeZ->node2 = pNodeA;

    /* ======================================================================================== */

    /* OptionBar / Other */
    pNodeParent->node1 = pNodeZ;
    pNodeParent->node2 = pNodeOptionBar;
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
  ViewportWindow* pViewportWindow = Panitent_GetActiveViewport();
  return ViewportWindow_GetDocument(pViewportWindow);
}

void Panitent_SetActiveViewport(ViewportWindow* pViewportWindow)
{
    g_app->m_pViewportWindow = pViewportWindow;
}

ViewportWindow* Panitent_GetActiveViewport()
{
    return g_app->m_pViewportWindow;
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

ViewportWindow* Panitent_CreateViewport()
{
  ViewportWindow* pViewportWindow = Panitent_GetActiveViewport();

  if (!pViewportWindow)
  {
    Panitent_SetActiveViewport(NULL);
  }

  return pViewportWindow;
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
  {
      return;
  }

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

  app->m_pPanitentWindow = PanitentWindow_Create((Application *)app);

  app->palette = Palette_Create();
}

void Panitent_CmdClearCanvas(struct PanitentApplication* app)
{
    Document* pDocument = Panitent_GetActiveDocument();
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    Canvas_Clear(pCanvas);
    Window_Invalidate((Window *)Panitent_GetActiveViewport());
}

void Panitent_CmdSaveFile(struct PanitentApplication* app)
{
    Document_Save(Panitent_GetActiveDocument());
}

PanitentApplication* Panitent_GetApp()
{
    return g_app;
}

WorkspaceContainer* Panitent_GetWorkspaceContainer()
{
    return g_app->m_pWorkspaceContainer;
}

Tool* Panitent_GetTool()
{
    return g_app->m_pTool;
}

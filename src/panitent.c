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
#include "util/tree.h"
#include "layerswindow.h"
#include "sharing/activitysharingmanager.h"
#include "sharing/activitystubdialog.h"
#include "propgriddialog.h"
#include "pntxml.h"
#include "experimental/docklib.h"
#include "util/assert.h"

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
void FetchSystemFont();

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
    LPWSTR* pszArgList;
    int nArgs;
    LPWSTR pszArgFile;
    WCHAR szModulePath[MAX_PATH];
    WCHAR szWorkDir[MAX_PATH];
    WCHAR szAppData[MAX_PATH];

    /* Request Common Controls v6 */
    INITCOMMONCONTROLSEX icex = { 0 };
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    struct PanitentApplication* app = PanitentApplication_Create();
    g_app = app;

    AddVectoredExceptionHandler(TRUE, PanitentUnhandledExceptionFilter);

    WindowingInit();

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
        {
            pszArgFile = pszArgList[0];
        }
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

    Panitent_SetActivityStatus(Panitent_GetApp(), L"Idle");

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
    Window_Show((Window*)app->m_pPanitentWindow, nCmdShow);

    if (pszArgFile)
    {
        Panitent_OpenFile(pszArgFile);
    }

    DockPanel_RegisterClass(hInstance);

    /* Experimental DockWindow */
    {
        DockHostWindow2* pDockHostWindow2 = NULL;
        pDockHostWindow2 = DockHostWindow2_Create(app);
        Window_CreateWindow(pDockHostWindow2, NULL);
        Window_Show(&pDockHostWindow2, SW_SHOW);
    }

    /* Application message loop.
     * Just pump inbound window messages and forward them to associated window
       procedure */
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
        TranslateMessage(&msg);
    }

    return (int)msg.wParam;
}

#ifdef _MSCVER
#pragma warning( pop )
#endif  /* _MSC_VER */

void Panitent_SaveSettingsFile(PNTSETTINGS* pPanitentSettings)
{
    FILE* fp = NULL;
    errno_t result = _wfopen_s(&fp, L"settings.dat", L"wb");
    assert(result == 0 && fp);
    if (result == 0 && fp)
    {
        fwrite(pPanitentSettings, sizeof(PNTSETTINGS), 1, fp);
        fclose(fp);
    }
}

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

    Panitent_SaveSettingsFile(pSettings);

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
    BOOL(*lpfnClassRegisterer)(HINSTANCE))
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

    bStatus = bStatus && AssertClassRegistration(hInstance, L"SettingsDialog", SettingsDialog_RegisterClass);
    bStatus = bStatus && AssertClassRegistration(hInstance, L"SettingsWindow", SettingsWindow_Register);
    bStatus = bStatus && AssertClassRegistration(hInstance, L"SwatchControl2", SwatchControl2_RegisterClass);
    bStatus = bStatus && AssertClassRegistration(hInstance, L"BrushSel", BrushSel_RegisterClass);

    return bStatus;
}

void Panitent_DockHostInit(DockHostWindow* pDockHostWindow, TreeNode* pNodeParent)
{
    DockData* pDockDataParent = DockData_Create(64, DGA_END | DGP_ABSOLUTE | DGD_VERTICAL, FALSE);
    pNodeParent->data = (void*)pDockDataParent;
    wcscpy_s(pDockDataParent->lpszName, MAX_PATH, L"Root");

    TreeNode* pNodeOptionBar = BinaryTree_AllocEmptyNode();

    DockData* pDockDataOptionBar = DockData_Create(0, DGA_END | DGP_ABSOLUTE | DGD_HORIZONTAL, FALSE);
    pNodeOptionBar->data = (void*)pDockDataOptionBar;
    wcscpy_s(pDockDataOptionBar->lpszName, MAX_PATH, L"OptionBar");
    OptionBarWindow* pOptionBarWindow = OptionBarWindow_Create((Application*)g_app);
    Window_CreateWindow((Window*)pOptionBarWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataOptionBar, (Window*)pOptionBarWindow);

    /* Main node */
    TreeNode* pNodeZ = BinaryTree_AllocEmptyNode();
    DockData* pDockDataZ = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataOptionBar, 0, sizeof(DockData));
    pNodeZ->data = (void*)pDockDataZ;
    pDockDataZ->dwStyle = DGA_END | DGP_ABSOLUTE | DGD_HORIZONTAL;
    pDockDataZ->iGripPos = 192;
    pDockDataZ->bShowCaption = FALSE;
    wcscpy_s(pDockDataZ->lpszName, MAX_PATH, L"Main");

    /* ======================================================================================== */

    /* Toolbox node */
    TreeNode* pNodeToolbox = BinaryTree_AllocEmptyNode();
    DockData* pDockDataToolbox = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataToolbox, 0, sizeof(DockData));
    pNodeToolbox->data = (void*)pDockDataToolbox;
    wcscpy_s(pDockDataToolbox->lpszName, MAX_PATH, L"Toolbox");
    ToolboxWindow* pToolboxWindow = ToolboxWindow_Create((struct Application*)g_app);
    hwndToolbox = Window_CreateWindow((Window*)pToolboxWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataToolbox, (Window*)pToolboxWindow);

    /* Viewport node */
    TreeNode* pNodeViewport = BinaryTree_AllocEmptyNode();
    DockData* pDockDataViewport = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataViewport, 0, sizeof(DockData));
    pNodeViewport->data = (void*)pDockDataViewport;
    wcscpy_s(pDockDataViewport->lpszName, MAX_PATH, L"WorkspaceContainer");
    WorkspaceContainer* pWorkspaceContainer = WorkspaceContainer_Create((Application*)g_app);
    HWND hWndWorkspaceContainer = Window_CreateWindow((Window*)pWorkspaceContainer, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataViewport, (Window*)pWorkspaceContainer);
    g_app->m_pWorkspaceContainer = pWorkspaceContainer;

    /* Toolbox/Viewport node */
    TreeNode* pNodeY = BinaryTree_AllocEmptyNode();
    DockData* pDockDataY = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataY, 0, sizeof(DockData));
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
    DockData* pDockDataGLWindow = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataGLWindow, 0, sizeof(DockData));
    pNodeGLWindow->data = (void*)pDockDataGLWindow;
    wcscpy_s(pDockDataGLWindow->lpszName, MAX_PATH, L"GLWindow");
    GLWindow* pGLWindow = GLWindow_Create((struct Application*)g_app);
    HWND hwndGLWindow = Window_CreateWindow((Window*)pGLWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataGLWindow, (Window*)pGLWindow);

    /* Create and pin palette window */
    TreeNode* pNodePalette = BinaryTree_AllocEmptyNode();
    DockData* pDockDataPalette = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataPalette, 0, sizeof(DockData));
    pNodePalette->data = (void*)pDockDataPalette;
    wcscpy_s(pDockDataPalette->lpszName, MAX_PATH, L"Palette");
    PaletteWindow* pPaletteWindow = PaletteWindow_Create((Application*)g_app, g_app->palette);
    hwndPalette = Window_CreateWindow((Window*)pPaletteWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataPalette, (Window*)pPaletteWindow);

    /* Create and pin layers window */
    TreeNode* pNodeLayers = BinaryTree_AllocEmptyNode();
    DockData* pDockDataLayers = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataLayers, 0, sizeof(DockData));
    pNodeLayers->data = (void*)pDockDataLayers;
    wcscpy_s(pDockDataLayers->lpszName, MAX_PATH, L"Layers");
    LayersWindow* pLayersWindow = LayersWindow_Create((Application*)g_app);
    HWND hwndLayers = Window_CreateWindow((Window*)pLayersWindow, NULL);
    DockData_PinWindow(pDockHostWindow, pDockDataLayers, (Window*)pLayersWindow);

    /* Palette/Layers node */
    TreeNode* pNodeB = BinaryTree_AllocEmptyNode();
    DockData* pDockDataB = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataB, 0, sizeof(DockData));
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
    DockData* pDockDataA = (DockData*)malloc(sizeof(DockData));
    memset(pDockDataA, 0, sizeof(DockData));
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

    WCHAR pszSource[] = L""
        L"<Root>"
        L"<Hello>World</Hello>"
        L"<This>"
        L"<Is>:-)</Is>"
        L"<An>:-0</An>"
        L"<Example>:-D</Example>"
        L"</This>"
        L"</Root>"
        ;

    struct XMLDocument* pXMLDocument = XML_ParseDocument(pszSource, wcslen(pszSource));

    if (!pXMLDocument)
    {
        Panitent_RaiseException(L"Could not parse pXMLDocument\n");
    }
    XMLNode* root = XMLDocument_GetRoot(pXMLDocument);

    /* Say Hello World :-) */
    XMLNode* pRootHello = XMLNode_GetChild(root, 0);
    XMLString* pxsHello = XMLNode_GetName(pRootHello);
    XMLString* pxsWorld = XMLNode_GetContent(pRootHello);

    /* Watch out: `xml_string_copy` will not 0-ternimate your `calloc` will :-) */
    PWSTR hello_0 = calloc(XMLString_Length(pxsHello) + 1, sizeof(WCHAR));
    PWSTR world_0 = calloc(XMLString_Length(pxsHello) + 1, sizeof(WCHAR));
    XMLString_Copy(pxsHello, hello_0, XMLString_Length(pxsHello));
    XMLString_Copy(pxsWorld, world_0, XMLString_Length(pxsWorld));

    wprintf_s(L"%s %s\n", hello_0, world_0);
    free(hello_0);
    free(world_0);

    /* Extract amout of Root/This children */
    XMLNode* pRootThis = XMLNode_GetChild(root, 1);
    wprintf_s(L"Root/This has %lu ppChildren\n", (unsigned long)XMLNode_GetChildrenCount(pRootThis));

    /* Remember to free the document or you'ss risk a memory leak */
    XMLDocument_Free(pXMLDocument, FALSE);
}

void FetchSystemFont()
{
    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof(NONCLIENTMETRICS);

    BOOL bResult = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
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

HFONT GetGuiFont()
{
    if (!hFontSys)
    {
        FetchSystemFont();
    }

    return hFontSys;
}

void SetGuiFont(HWND hwnd)
{
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFontSys, MAKELPARAM(FALSE, 0));
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

PNTSETTINGS* Panitent_GetSettings()
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

    Document* doc = Panitent_GetActiveDocument();
    if (!doc)
        return;

    Canvas* canvas = Document_GetCanvas(doc);
    if (!canvas)
        return;

    unsigned char* pData = malloc(canvas->buffer_size);
    memset(pData, 0, canvas->buffer_size);
    if (!pData)
    {
        return;
    }

    ZeroMemory(pData, canvas->buffer_size);
    memcpy(pData, canvas->buffer, canvas->buffer_size);

    for (size_t y = 0; y < (size_t)canvas->height; y++) {
        for (size_t x = 0; x < (size_t)canvas->width; x++) {
            uint8_t* pixel = pData + (x + y * canvas->width) * 4;

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

PanitentApplication* PanitentApplication_Create()
{
    PanitentApplication* pPanitentApplication = (PanitentApplication*)malloc(sizeof(PanitentApplication));

    if (pPanitentApplication)
    {
        memset(pPanitentApplication, 0, sizeof(PanitentApplication));
        PanitentApplication_Init(pPanitentApplication);
    }

    return pPanitentApplication;
}

void PanitentApplication_Init(struct PanitentApplication* app)
{
    Application_Init(&app->base);

    app->m_pPanitentWindow = PanitentWindow_Create((Application*)app);
    app->palette = Palette_Create();
    app->m_pActivitySharingManager = ActivitySharingManager_Create();
}

void Panitent_CmdClearCanvas(struct PanitentApplication* app)
{
    Document* pDocument = Panitent_GetActiveDocument();
    Canvas* pCanvas = Document_GetCanvas(pDocument);
    Canvas_Clear(pCanvas);
    Window_Invalidate((Window*)Panitent_GetActiveViewport());
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

Tool* Panitent_GetSelectedTool()
{
    return g_app->m_pTool;
}

void Panitent_CmdShowActivityDialog(PanitentApplication* pApp)
{
    ActivityStubDialog* pActivityStubDialog = ActivityStubDialog_Create(pApp);
    ActivitySharingManager_AddClient(pApp->m_pActivitySharingManager, &pActivityStubDialog->m_activitySharingClient);
    HWND hWnd = Dialog_CreateWindow(pActivityStubDialog, IDD_ACTIVITYSTUB, NULL, FALSE);
    ShowWindow(hWnd, SW_SHOW);
}

void Panitent_SetActivityStatus(PanitentApplication* pApp, PCWSTR pszStatusMessage)
{
    ActivitySharingManager_SetStatus(pApp->m_pActivitySharingManager, pszStatusMessage);
}

void Panitent_SetActivityStatusF(PanitentApplication* pApp, PCWSTR pszFormat, ...)
{
    va_list argp;
    va_start(argp, pszFormat);

    WCHAR szMessage[256];
    vswprintf_s(szMessage, 256, pszFormat, argp);

    va_end(argp);

    Panitent_SetActivityStatus(pApp, szMessage);
}

void Panitent_RaiseException(PCWSTR pszExceptionMessage)
{
    MessageBox(NULL, pszExceptionMessage, NULL, MB_ICONERROR);
}

void Panitent_CmdShowPropertyGridDialog(PanitentApplication* pApp)
{
    PropertyGridDialog* pPropertyGridDialog = PropertyGridDialog_Create(pApp);
    HWND hDlg = Dialog_CreateWindow(pPropertyGridDialog, IDD_PROPERTYGRIDDLG, NULL, FALSE);
    ShowWindow(hDlg, SW_SHOW);
}

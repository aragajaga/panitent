#include "precomp.h"

#include "win32/application.h"
#include "win32/window.h"

#include "panitentapp.h"
#include "panitentwindow.h"
#include "dockshell.h"

#include "option_bar.h"
#include "toolbox.h"
#include "workspacecontainer.h"
#include "glwindow.h"
#include "palette_window.h"
#include "layerswindow.h"
#include "resource.h"
#include <time.h>
#include "document.h"
#include "canvas.h"
#include "brush.h"
#include "propgriddialog.h"
#include "sharing/activitysharingmanager.h"
#include "sharing/activitystubdialog.h"
#include "log_window.h"
#include "rbhashmapviz.h"
#include "new.h"
#include "log.h"
#include "binview/binviewdlg.h"
#include "grimstroke/shapecontext.h"
#include "grimstroke/wushapestrategy.h"
#include "grimstroke/bresenhamshapestrategy.h"

#include "grimstroke/pixelbuffer.h"
#include "grimstroke/polygon.h"
#include "util/assert.h"
#include "win32/util.h"
#include "lua_scripting.h"
#include "history.h"
#include "viewport.h"
#include "aboutbox.h"
#include "color_context.h"
#include "settings_wnd.h"

#include "verifycheck.h"
#include "playground.h"

#include <shellapi.h>

void PanitentApp_RegisterCommands(PanitentApp* pPanitentApp);

typedef enum _PanitentFilterProjection
{
    PanitentFilterProjection_LatLong = 0,
    PanitentFilterProjection_LatLongFront,
    PanitentFilterProjection_Orthographic,
    PanitentFilterProjection_Equidistant,
    PanitentFilterProjection_Equisolid,
    PanitentFilterProjection_Stereographic,
    PanitentFilterProjection_SquareBulge
} PanitentFilterProjection;

typedef struct _PanitentFilterVector3
{
    float x;
    float y;
    float z;
} PanitentFilterVector3;

static float PanitentApp_FilterClampSigned1(float value);
static float PanitentApp_FilterClamp01(float value);
static float PanitentApp_FilterLerp(float a, float b, float t);
static BOOL PanitentApp_FilterLatLongToDirection(float x, float y, PanitentFilterVector3* pDirection);
static void PanitentApp_FilterDirectionToLatLong(const PanitentFilterVector3* pDirection, float* x, float* y);
static BOOL PanitentApp_FilterFrontLatLongToDirection(float x, float y, PanitentFilterVector3* pDirection);
static BOOL PanitentApp_FilterDirectionToFrontLatLong(const PanitentFilterVector3* pDirection, float* x, float* y);
static BOOL PanitentApp_FilterMirrorBallToDirection(float u, float v, PanitentFilterVector3* pDirection);
static void PanitentApp_FilterDirectionToMirrorBall(const PanitentFilterVector3* pDirection, float hintX, float hintY, float* u, float* v);
static void PanitentApp_FilterSquareToDisk(float x, float y, float* u, float* v);
static BOOL PanitentApp_FilterDiskToSquare(float u, float v, float* x, float* y);
static void PanitentApp_FilterSquareBulgeToDisk(float x, float y, float* u, float* v);
static BOOL PanitentApp_FilterDiskToSquareBulge(float u, float v, float* x, float* y);
static float PanitentApp_FilterProjectRadius(PanitentFilterProjection projection, float radius);
static float PanitentApp_FilterUnprojectRadius(PanitentFilterProjection projection, float radius);
static uint32_t PanitentApp_FilterSampleBilinear(const Canvas* pCanvas, float x, float y);
static void PanitentApp_ApplySpherizeFilter(PanitentApp* pPanitentApp, BOOL fUnspherize, PanitentFilterProjection projection);
static void PanitentApp_CmdFilterSpherizeLatLong(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeLatLong(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterSpherizeLatLongFront(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeLatLongFront(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterSpherizeOrthographic(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeOrthographic(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterSpherizeEquidistant(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeEquidistant(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterSpherizeEquisolid(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeEquisolid(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterSpherizeStereographic(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeStereographic(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterSpherizeSquareBulge(PanitentApp* pPanitentApp);
static void PanitentApp_CmdFilterUnspherizeSquareBulge(PanitentApp* pPanitentApp);

void PanitentApp_Init(PanitentApp* pPanitentApp)
{
    Application_Init(&pPanitentApp->base);

    AppCmd_Init(&pPanitentApp->m_appCmd);
    PanitentApp_RegisterCommands(pPanitentApp);
    pPanitentApp->pPanitentWindow = PanitentWindow_Create(pPanitentApp);

    pPanitentApp->palette = Palette_Create();
    pPanitentApp->m_pActivitySharingManager = ActivitySharingManager_Create();
    pPanitentApp->m_pWorkspaceContainer = WorkspaceContainer_Create();
    Panitent_DefaultSettings(&pPanitentApp->m_settings);
    Panitent_ReadSettings(&pPanitentApp->m_settings);
    InitColorContext();
    InitializeBrushList();

    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    pPanitentApp->m_hFont = CreateFontIndirect(&ncm.lfMessageFont);
    StringCchCopyW(
        pPanitentApp->m_szTextToolFontFace,
        ARRAYSIZE(pPanitentApp->m_szTextToolFontFace),
        ncm.lfMessageFont.lfFaceName[0] != L'\0' ? ncm.lfMessageFont.lfFaceName : L"Segoe UI");
    pPanitentApp->m_nTextToolFontPx = 24;
    pPanitentApp->m_pShapeContext = ShapeContext_Create();
    ShapeContext_SetStrategy(pPanitentApp->m_pShapeContext, WuShapeStrategy_Create());

    // PlaygroundDlg playgroundDlg;
    // PlaygroundDlg_Init(&playgroundDlg);
    // Dialog_CreateWindow(&playgroundDlg, IDD_PLAYGROUND, NULL, TRUE);
}

PanitentApp* PanitentApp_Create()
{
    PanitentApp* pPanitentApp = (PanitentApp*)malloc(sizeof(PanitentApp));
    if (pPanitentApp)
    {
        memset(pPanitentApp, 0, sizeof(PanitentApp));
        PanitentApp_Init(pPanitentApp);
    }

    return pPanitentApp;
}

int WINAPI VerifyCheckThreadProc(PVOID pParam)
{
    PanitentWindow* pPanitentWindow = PanitentApp_GetWindow(PanitentApp_Instance());

    while (TRUE)
    {
        Sleep(5000);
        if (pPanitentWindow)
        {
            VerifyCheckDlg* pVerifyCheckDlg = malloc(sizeof(VerifyCheckDlg));
            VerifyCheckDlg_Init(pVerifyCheckDlg);
        
            Dialog_CreateWindow(pVerifyCheckDlg, IDD_VERIFYCHECK, Window_GetHWND((Window*)pPanitentWindow), TRUE);
            // Window_Show(&verifyCheckDlg, SW_SHOW);

            free(pVerifyCheckDlg);
        }
    }

    return 0;
}

int PanitentApp_Run(PanitentApp* pPanitentApp)
{
    srand((unsigned int)time(NULL));

    Window_CreateWindow((Window*)pPanitentApp->pPanitentWindow, NULL);

    PanitentApp* pPanitentApp2 = PanitentApp_Instance();
    // HANDLE hVerifyCheckThread = CreateThread(NULL, 0, VerifyCheckThreadProc, NULL, 0, NULL);

    return Application_Run(pPanitentApp);
}

TreeNode* CreateToolboxNode(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
    TreeNode* pNodeToolbox = DockShell_CreatePanelNode(L"Toolbox");
    if (pNodeToolbox && pNodeToolbox->data)
    {
        DockData* pDockDataToolbox = (DockData*)pNodeToolbox->data;
        ToolboxWindow* pToolboxWindow = ToolboxWindow_Create((Application*)pPanitentApp);
        Window_CreateWindow((Window*)pToolboxWindow, NULL);
        DockData_PinWindow(pDockHostWindow, pDockDataToolbox, (Window*)pToolboxWindow);
    }

    return pNodeToolbox;
}

TreeNode* CreateViewportNode(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
    TreeNode* pNodeViewport = DockShell_CreateWorkspaceNode();
    if (pNodeViewport && pNodeViewport->data)
    {
        DockData* pDockDataViewport = (DockData*)pNodeViewport->data;
        WorkspaceContainer* pWorkspaceContainer = WorkspaceContainer_Create((Application*)pPanitentApp);
        HWND hWndWorkspaceContainer = Window_CreateWindow((Window*)pWorkspaceContainer, NULL);
        DockData_PinWindow(pDockHostWindow, pDockDataViewport, (Window*)pWorkspaceContainer);
        pDockDataViewport->bShowCaption = FALSE;
        pPanitentApp->m_pWorkspaceContainer = pWorkspaceContainer;
    }
    
    return pNodeViewport;
}

TreeNode* CreateGLWindowNode(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
    TreeNode* pNodeGLWindow = DockShell_CreatePanelNode(L"GLWindow");
    if (pNodeGLWindow && pNodeGLWindow->data)
    {
        DockData* pDockDataGLWindow = (DockData*)pNodeGLWindow->data;
        GLWindow* pGLWindow = GLWindow_Create((struct Application*)pPanitentApp);
        HWND hwndGLWindow = Window_CreateWindow((Window*)pGLWindow, NULL);
        DockData_PinWindow(pDockHostWindow, pDockDataGLWindow, (Window*)pGLWindow);
    }

    return pNodeGLWindow;
}

TreeNode* CreatePaletteWindowNode(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
    TreeNode* pNodePalette = DockShell_CreatePanelNode(L"Palette");
    if (pNodePalette && pNodePalette->data)
    {
        DockData* pDockDataPalette = (DockData*)pNodePalette->data;
        PaletteWindow* pPaletteWindow = PaletteWindow_Create(pPanitentApp->palette);
        HWND hwndPalette = Window_CreateWindow((Window*)pPaletteWindow, NULL);
        DockData_PinWindow(pDockHostWindow, pDockDataPalette, (Window*)pPaletteWindow);
    }
    
    return pNodePalette;
}

TreeNode* CreateLayersWindowNode(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
    TreeNode* pNodeLayers = DockShell_CreatePanelNode(L"Layers");
    if (pNodeLayers && pNodeLayers->data)
    {
        DockData* pDockDataLayers = (DockData*)pNodeLayers->data;
        LayersWindow* pLayersWindow = LayersWindow_Create((Application*)pPanitentApp);
        HWND hwndLayers = Window_CreateWindow((Window*)pLayersWindow, NULL);
        DockData_PinWindow(pDockHostWindow, pDockDataLayers, (Window*)pLayersWindow);
    }

    return pNodeLayers;
}

TreeNode* CreateOptionBarNode(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
    TreeNode* pNodeOptionBar = DockShell_CreatePanelNode(_T("Option Bar"));
    if (pNodeOptionBar && pNodeOptionBar->data)
    {
        DockData* pDockDataOptionBar = (DockData*)pNodeOptionBar->data;
        OptionBarWindow* pOptionBarWindow = OptionBarWindow_Create();
        HWND hwndLayers = Window_CreateWindow((Window*)pOptionBarWindow, NULL);
        PanitentApp_SetOptionBar(pPanitentApp, pOptionBarWindow);
        OptionBarWindow_SyncTool(pOptionBarWindow, PanitentApp_GetTool(pPanitentApp));
        DockData_PinWindow(pDockHostWindow, pDockDataOptionBar, (Window*)pOptionBarWindow);
    }

    return pNodeOptionBar;
}

void PanitentApp_DockHostInit(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, TreeNode* pNodeParent)
{
    DockShellMetrics shellMetrics = { 220, 300, 72, 72 };

    /*
     * Dock shell layout:
     *   - Center: workspace/document area
     *   - Edge zones: DockZone.Left/Right/Top/Bottom
     *   - Any runtime docking goes into these zones, not around whole root.
     */
    TreeNode* pNodeWorkspace = CreateViewportNode(pPanitentApp, pDockHostWindow);
    TreeNode* pNodeToolbox = CreateToolboxNode(pPanitentApp, pDockHostWindow);
    TreeNode* pNodeGLWindow = CreateGLWindowNode(pPanitentApp, pDockHostWindow);
    TreeNode* pNodePalette = CreatePaletteWindowNode(pPanitentApp, pDockHostWindow);
    TreeNode* pNodeLayers = CreateLayersWindowNode(pPanitentApp, pDockHostWindow);
    TreeNode* pNodeOptionBar = CreateOptionBarNode(pPanitentApp, pDockHostWindow);

    TreeNode* pZoneLeft = DockShell_CreateZoneNode(DKS_LEFT);
    TreeNode* pZoneRight = DockShell_CreateZoneNode(DKS_RIGHT);
    TreeNode* pZoneTop = DockShell_CreateZoneNode(DKS_TOP);
    TreeNode* pZoneBottom = DockShell_CreateZoneNode(DKS_BOTTOM);

    DockShell_InitRootNode(pNodeParent);

    if (pZoneLeft)
    {
        DockShell_AppendPanelToZone(pZoneLeft, pNodeToolbox);
    }

    if (pZoneRight)
    {
        DockShell_AppendPanelToZone(pZoneRight, pNodeGLWindow);
        DockShell_AppendPanelToZone(pZoneRight, pNodePalette);
        DockShell_AppendPanelToZone(pZoneRight, pNodeLayers);
    }

    if (pZoneBottom)
    {
        DockShell_AppendPanelToZone(pZoneBottom, pNodeOptionBar);
    }

    if (!DockShell_BuildMainLayout(
        pNodeParent,
        pNodeWorkspace,
        pZoneLeft,
        pZoneRight,
        pZoneTop,
        pZoneBottom,
        &shellMetrics))
    {
        DockShell_BuildWorkspaceOnlyLayout(pNodeParent, pNodeWorkspace);
    }
}

Palette* PanitentApp_GetPalette(PanitentApp* pPanitentApp)
{
    return pPanitentApp->palette;
}

PanitentApp* PanitentApp_Instance()
{
    static PanitentApp* s_pPanitentApp;
    if (!s_pPanitentApp)
    {
        s_pPanitentApp = PanitentApp_Create();
    }

    return s_pPanitentApp;
}

HFONT PanitentApp_GetUIFont(PanitentApp* pPanitentApp)
{
    return pPanitentApp->m_hFont;
}

PNTSETTINGS* PanitentApp_GetSettings(PanitentApp* pPanitentApp)
{
    return pPanitentApp ? &pPanitentApp->m_settings : NULL;
}

void PanitentApp_SetActiveViewport(PanitentApp* pPanitentApp, ViewportWindow* pViewportWindow)
{
    pPanitentApp->m_pViewportWindow = pViewportWindow;
}

ViewportWindow* PanitentApp_GetActiveViewport(PanitentApp* pPanitentApp)
{
    return pPanitentApp->m_pViewportWindow;
}

Document* PanitentApp_GetActiveDocument(PanitentApp* pPanitentApp)
{
    ViewportWindow *pViewportWindow = PanitentApp_GetActiveViewport(pPanitentApp);
    if (pViewportWindow == NULL)
        return NULL;

    return ViewportWindow_GetDocument(pViewportWindow);
}

ActivitySharingManager* PanitentApp_GetActivitySharingManager(PanitentApp* pPanitentApp)
{
    return pPanitentApp->m_pActivitySharingManager;
}

ViewportWindow* PanitentApp_GetCurrentViewport(PanitentApp* pPanitentApp)
{
    return WorkspaceContainer_GetCurrentViewport(pPanitentApp->m_pWorkspaceContainer);
}

PanitentWindow* PanitentApp_GetWindow(PanitentApp* pPanitentApp)
{
    return pPanitentApp->pPanitentWindow;
}

WorkspaceContainer* PanitentApp_GetWorkspaceContainer(PanitentApp* pPanitentApp)
{
    return pPanitentApp->m_pWorkspaceContainer;
}

OptionBarWindow* PanitentApp_GetOptionBar(PanitentApp* pPanitentApp)
{
    return pPanitentApp ? pPanitentApp->m_pOptionBarWindow : NULL;
}

void PanitentApp_SetOptionBar(PanitentApp* pPanitentApp, OptionBarWindow* pOptionBarWindow)
{
    if (pPanitentApp)
    {
        pPanitentApp->m_pOptionBarWindow = pOptionBarWindow;
    }
}

void PanitentApp_SetTool(PanitentApp* pPanitentApp, Tool* pTool)
{
    ViewportWindow* pViewportWindow = PanitentApp_GetActiveViewport(pPanitentApp);
    if (pViewportWindow && PanitentApp_GetTool(pPanitentApp) != pTool)
    {
        ViewportWindow_CommitTextOverlay(pViewportWindow);
    }

    pPanitentApp->m_pTool = pTool;

    if (pPanitentApp->m_pOptionBarWindow)
    {
        OptionBarWindow_SyncTool(pPanitentApp->m_pOptionBarWindow, pTool);
    }
}

Tool* PanitentApp_GetTool(PanitentApp* pPanitentApp)
{
    return pPanitentApp->m_pTool;
}

#include "grimstroke/plotter.h"

ShapeContext* PanitentApp_GetShapeContext(PanitentApp* pPanitentApp)
{
    return pPanitentApp ? pPanitentApp->m_pShapeContext : NULL;
}

PCWSTR PanitentApp_GetTextToolFontFace(PanitentApp* pPanitentApp)
{
    if (!pPanitentApp || pPanitentApp->m_szTextToolFontFace[0] == L'\0')
    {
        return L"Segoe UI";
    }

    return pPanitentApp->m_szTextToolFontFace;
}

int PanitentApp_GetTextToolFontPx(PanitentApp* pPanitentApp)
{
    return pPanitentApp ? max(1, pPanitentApp->m_nTextToolFontPx) : 24;
}

void PanitentApp_SetTextToolFontFace(PanitentApp* pPanitentApp, PCWSTR pszFaceName)
{
    if (!pPanitentApp || !pszFaceName || pszFaceName[0] == L'\0')
    {
        return;
    }

    StringCchCopyW(
        pPanitentApp->m_szTextToolFontFace,
        ARRAYSIZE(pPanitentApp->m_szTextToolFontFace),
        pszFaceName);

    ViewportWindow_RefreshTextOverlayStyle(PanitentApp_GetActiveViewport(pPanitentApp));
    if (pPanitentApp->m_pOptionBarWindow)
    {
        OptionBarWindow_SyncTool(pPanitentApp->m_pOptionBarWindow, pPanitentApp->m_pTool);
    }
}

void PanitentApp_SetTextToolFontPx(PanitentApp* pPanitentApp, int nFontPx)
{
    if (!pPanitentApp)
    {
        return;
    }

    pPanitentApp->m_nTextToolFontPx = min(256, max(1, nFontPx));
    ViewportWindow_RefreshTextOverlayStyle(PanitentApp_GetActiveViewport(pPanitentApp));
    if (pPanitentApp->m_pOptionBarWindow)
    {
        OptionBarWindow_SyncTool(pPanitentApp->m_pOptionBarWindow, pPanitentApp->m_pTool);
    }
}

BOOL PanitentApp_OpenDroppedFiles(PanitentApp* pPanitentApp, HDROP hDrop)
{
    UINT nFiles = 0;
    BOOL bOpenedAny = FALSE;

    UNREFERENCED_PARAMETER(pPanitentApp);

    if (!hDrop)
    {
        return FALSE;
    }

    nFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
    for (UINT i = 0; i < nFiles; ++i)
    {
        UINT cchPath = DragQueryFileW(hDrop, i, NULL, 0);
        WCHAR* pszPath = NULL;

        if (cchPath == 0)
        {
            continue;
        }

        pszPath = (WCHAR*)calloc((size_t)cchPath + 1, sizeof(WCHAR));
        if (!pszPath)
        {
            continue;
        }

        if (DragQueryFileW(hDrop, i, pszPath, cchPath + 1) > 0)
        {
            Document_OpenFile(pszPath);
            bOpenedAny = TRUE;
        }

        free(pszPath);
    }

    return bOpenedAny;
}

void PanitentApp_CmdNewFile(PanitentApp* pPanitentApp)
{
    LogMessage(LOGENTRY_TYPE_INFO, L"MAIN", L"New File menu issued");
    
    if (0)
    {
        PanitentWindow* pPanitentWindow = NULL;
        pPanitentWindow = PanitentApp_GetWindow(pPanitentApp);
        if (pPanitentWindow == NULL)
        {
            LogMessage(LOGENTRY_TYPE_ERROR, L"MAIN", L"Failed to get parent appframe (PanitentWindow* PanitentApp_GetWindow()) during the creation of document setup dialog.");
            return;
        }

        HWND hwndAppFrame = Window_GetHWND((Window*)pPanitentWindow);
        if (hwndAppFrame == NULL || !IsWindow(hwndAppFrame))
        {
            LogMessage(LOGENTRY_TYPE_ERROR, L"MAIN", L"Failed to get appframe (Window_GetHWND((Window*)PanitentWindow*)) HWND during the creation of document setup dialog.");
            return;
        }

        HINSTANCE hInstanceApp = NULL;
        hInstanceApp = (HINSTANCE)GetWindowLongPtr(hwndAppFrame, GWLP_HINSTANCE);
        if (!hInstanceApp)
        {
            LogMessage(LOGENTRY_TYPE_WARNING, L"MAIN", L"AppFrame's GWLP_HINSTANCE is NULL, it's an abnormal behavior. Falling back to the GetModuleHandle(NULL)");
            hInstanceApp = GetModuleHandle(NULL);
        }

        if (!hInstanceApp)
        {
            /* Something is completely broken */
            DebugBreak();
            return;
        }

        BOOL bResult = DocumentSetupDialog_RegisterClass(hInstanceApp);
        if (!bResult)
        {
            LogMessage(LOGENTRY_TYPE_ERROR, L"MAIN", L"Failed to register DocumentSetupDialogV1 class during the creation of document setup dialog.");
            return;
        }

        RECT rcDocSetupWnd = { 0 };
        rcDocSetupWnd.right = 320;
        rcDocSetupWnd.bottom = 240;
        AdjustWindowRect(&rcDocSetupWnd, WS_OVERLAPPEDWINDOW, FALSE);

        int width = RECTWIDTH(&rcDocSetupWnd);
        int height = RECTHEIGHT(&rcDocSetupWnd);

        RECT rcWorkarea = { 0 };

#if 0   /* No monitor info */
        SystemParametersInfo(SPI_GETWORKAREA, sizeof(RECT), &rcWorkarea, 0);
#else
        /* With monitor info */


        do {
            BOOL bFromCursor = TRUE;
            DWORD dwFlags = MONITOR_DEFAULTTONEAREST;
            HMONITOR hMonitor = NULL;

            if (bFromCursor)
            {
                POINT pt;
                GetCursorPos(&pt);
                hMonitor = MonitorFromPoint(pt, dwFlags);
            }
            else {
                if (!IsWindow(hwndAppFrame))
                    break;
                hMonitor = MonitorFromWindow(hwndAppFrame, dwFlags);
            }

            if (!hMonitor) break;

            MONITORINFO mi = { 0 };
            mi.cbSize = sizeof(MONITORINFO);

            if (!GetMonitorInfo(hMonitor, &mi)) break;

            CopyRect(&rcWorkarea, &mi.rcWork);
            break;
        } while (FALSE);
#endif

        /* Center window in workarea */
        int x = rcWorkarea.left + (RECTWIDTH(&rcWorkarea) - width) / 2;
        int y = rcWorkarea.top + (RECTHEIGHT(&rcWorkarea) - height) / 2;

        HWND hwndDocSetup = NULL;
        hwndDocSetup = CreateWindowEx(0, L"{69097ff4-7bc3-4339-9164-2b4b5d88b601}", L"New", WS_OVERLAPPEDWINDOW,
            x, y, width, height, hwndAppFrame, NULL, hInstanceApp, NULL);
        if (hwndDocSetup == NULL || !IsWindow(hwndDocSetup))
        {
            LogMessage(LOGENTRY_TYPE_ERROR, L"MAIN", L"Failed to create DocumentSetupDialogV1 window.");
            return;
        }

        ShowWindow(hwndDocSetup, SW_NORMAL);
        UpdateWindow(hwndDocSetup);
    }
    else {
        NewFileDialog(pPanitentApp->pPanitentWindow);
    }    
}

void PanitentApp_CmdOpenFile(PanitentApp* pPanitentApp)
{
    Document_Open(NULL);
}

void PanitentApp_CmdSaveFile(PanitentApp* pPanitentApp)
{
    Document_Save(PanitentApp_GetActiveDocument(pPanitentApp));
}

void PanitentApp_CmdRunScript(PanitentApp* pPanitentApp)
{
    // Lua_RunScript(L"init.lua");
}

void PanitentApp_CmdBinView(PanitentApp* pPanitentApp)
{
    BinViewDialog* binView = (BinViewDialog*)malloc(sizeof(BinViewDialog));
    BinViewDialog_Init(binView);
    HWND hDlg = Dialog_CreateWindow(binView, IDD_BINVIEW, NULL, FALSE);
    ShowWindow(hDlg, SW_SHOW);
}

void PanitentApp_CmdClearCanvas(PanitentApp* pPanitentApp)
{
    if (pPanitentApp == NULL)
        return;

    Document* pDocument = PanitentApp_GetActiveDocument(pPanitentApp);
    if (pDocument == NULL)
        return;

    Canvas* pCanvas = Document_GetCanvas(pDocument);
    if (pCanvas == NULL)
        return;
    
    Canvas_Clear(pCanvas);
    Window_Invalidate(PanitentApp_GetActiveViewport(pPanitentApp));
}

static void PanitentApp_CmdFilterSpherizeLatLong(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_LatLong);
}

static void PanitentApp_CmdFilterUnspherizeLatLong(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_LatLong);
}

static void PanitentApp_CmdFilterSpherizeLatLongFront(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_LatLongFront);
}

static void PanitentApp_CmdFilterUnspherizeLatLongFront(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_LatLongFront);
}

static void PanitentApp_CmdFilterSpherizeOrthographic(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_Orthographic);
}

static void PanitentApp_CmdFilterUnspherizeOrthographic(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_Orthographic);
}

static void PanitentApp_CmdFilterSpherizeEquidistant(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_Equidistant);
}

static void PanitentApp_CmdFilterUnspherizeEquidistant(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_Equidistant);
}

static void PanitentApp_CmdFilterSpherizeEquisolid(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_Equisolid);
}

static void PanitentApp_CmdFilterUnspherizeEquisolid(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_Equisolid);
}

static void PanitentApp_CmdFilterSpherizeStereographic(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_Stereographic);
}

static void PanitentApp_CmdFilterUnspherizeStereographic(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_Stereographic);
}

static void PanitentApp_CmdFilterSpherizeSquareBulge(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, FALSE, PanitentFilterProjection_SquareBulge);
}

static void PanitentApp_CmdFilterUnspherizeSquareBulge(PanitentApp* pPanitentApp)
{
    PanitentApp_ApplySpherizeFilter(pPanitentApp, TRUE, PanitentFilterProjection_SquareBulge);
}

void PanitentApp_CmdShowLog(PanitentApp* pPanitentApp)
{
    LogWindow* pLogWindow = NULL;
    pLogWindow = (LogWindow*)LogWindow_Create();

    if (pLogWindow) {
        Window_CreateWindow((Window*)pLogWindow, NULL);
    }
    Window_Show((Window*)pLogWindow, SW_SHOW);
}

void PanitentApp_CmdShowRbTreeViz(PanitentApp* pPanitentApp)
{
    RBHashMapVizDialog* pRBHashMapVizDialog = NULL;
    pRBHashMapVizDialog = RBHashMapVizDialog_Create();
    Dialog_CreateWindow(pRBHashMapVizDialog, IDD_RBTREEVIZ, NULL, TRUE);
}

void PanitentApp_CmdClipboardExport(PanitentApp* pPanitentApp) {
    HWND hWndPanitent = NULL;
    hWndPanitent = Window_GetHWND(pPanitentApp->pPanitentWindow);

    if (!OpenClipboard(hWndPanitent)) {
        return;
    }
    EmptyClipboard();

    Document* doc = PanitentApp_GetActiveDocument(pPanitentApp);
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

    SetClipboardData(CF_BITMAP, hBitmap);
    CloseClipboard();
}

void PanitentApp_CmdShowSettings(PanitentApp* pPanitentApp)
{
    UNREFERENCED_PARAMETER(pPanitentApp);
    SettingsWindow_Show();
}

void PanitentApp_CmdShowPropertyGridDialog(PanitentApp* pPanitentApp)
{
    PropertyGridDialog* pPropertyGridDialog = PropertyGridDialog_Create(pPanitentApp);
    HWND hDlg = Dialog_CreateWindow(pPropertyGridDialog, IDD_PROPERTYGRIDDLG, NULL, FALSE);
    ShowWindow(hDlg, SW_SHOW);
}

void PanitentApp_CmdShowActivityDialog(PanitentApp* pPanitentApp)
{
    ActivityStubDialog* pActivityStubDialog = ActivityStubDialog_Create(pPanitentApp);
    ActivitySharingManager_AddClient(pPanitentApp->m_pActivitySharingManager, &pActivityStubDialog->m_activitySharingClient);
    HWND hWnd = Dialog_CreateWindow(pActivityStubDialog, IDD_ACTIVITYSTUB, NULL, FALSE);
    ShowWindow(hWnd, SW_SHOW);
}

void PanitentApp_SetActivityStatus(PanitentApp* pPanitentApp, PCWSTR pszStatusMessage)
{
    ActivitySharingManager_SetStatus(pPanitentApp->m_pActivitySharingManager, pszStatusMessage);
}

void PanitentApp_SetActivityStatusF(PanitentApp* pPanitentApp, PCWSTR pszFormat, ...)
{
    va_list argp;
    va_start(argp, pszFormat);

    WCHAR szMessage[256];
    vswprintf_s(szMessage, 256, pszFormat, argp);

    va_end(argp);

    PanitentApp_SetActivityStatus(pPanitentApp, szMessage);
}

void PanitentApp_CmdDisplayPixelBuffer(PanitentApp* pPanitentApp)
{
    PolygonPath* pPolygonPath = PolygonPath_Create();
    PolygonPath_AddPoint(pPolygonPath, 50, 50);
    PolygonPath_AddPoint(pPolygonPath, 100, 50);
    PolygonPath_AddPoint(pPolygonPath, 100, 100);
    PolygonPath_AddPoint(pPolygonPath, 50, 100);
    // PolygonPath_Enclose(pPolygonPath);


    PixelBuffer pixelBuffer;
    PixelBuffer_Init(&pixelBuffer, 128, 128);
    Color bgColor;
    bgColor.r = 0xFF;
    bgColor.g = 0xFF;
    bgColor.b = 0xFF;
    bgColor.a = 0xFF;
    PixelBuffer_Clear(&pixelBuffer, bgColor);

    Color fgColor;
    fgColor.b = 0xFF;
    fgColor.g = 0x00;
    fgColor.r = 0x00;
    fgColor.a = 0xFF;
    Polygon_DrawOnPixelBuffer(pPolygonPath, &pixelBuffer, fgColor);

    
    unsigned char* pData = PixelBuffer_GetData(&pixelBuffer);
    int width = PixelBuffer_GetWidth(&pixelBuffer);
    int height = PixelBuffer_GetHeight(&pixelBuffer);
    BITMAPINFO bmi = { 0 };
    Win32_InitGdiBitmapInfo32Bpp(&bmi, width, height);
    
    void* pBitmapData = NULL;
    HBITMAP hBitmap = CreateDIBSection(GetDC(HWND_DESKTOP), &bmi, DIB_RGB_COLORS, &pBitmapData, NULL, 0);
    ASSERT(hBitmap);

    int rowSize = width * 4;
    for (int y = 0; y < height; ++y)
    {
        unsigned char* srcRow = pData + (height - 1 - y) * rowSize;
        unsigned char* dstRow = (unsigned char*)pBitmapData + y * rowSize;
        memcpy(dstRow, srcRow, rowSize);
    }

    HDC hdcBm = CreateCompatibleDC(GetDC(HWND_DESKTOP));
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcBm, hBitmap);
    BitBlt(GetDC(HWND_DESKTOP), 0, 0, width, height, hdcBm, 0, 0, SRCCOPY);
    SelectObject(hdcBm, hOldBm);
    DeleteObject(hBitmap);
    DeleteDC(hdcBm);


    FILE* fp = fopen("pixbuftest.bin", "wb");
    if (fp)
    {
        unsigned char* pData = PixelBuffer_GetData(&pixelBuffer);
        int width = PixelBuffer_GetWidth(&pixelBuffer);
        int height = PixelBuffer_GetHeight(&pixelBuffer);
        fwrite(pData, width * height * 4, 1, fp);
        fclose(fp);
    }
}

void PanitentApp_CmdClose(PanitentApp* pPanitentApp)
{
    PostQuitMessage(0);
}

void PanitentApp_CmdUndo(PanitentApp* pPanitentApp)
{
    History_Undo(PanitentApp_GetActiveDocument(pPanitentApp));
}

void PanitentApp_CmdRedo(PanitentApp* pPanitentApp)
{
    History_Redo(PanitentApp_GetActiveDocument(pPanitentApp));
}

void PanitentApp_CmdAbout(PanitentApp* pPanitentApp)
{
    PanitentWindow* pPanitentWindow = PanitentApp_GetWindow(pPanitentApp);
    HWND hMainWnd = Window_GetHWND((Window*)pPanitentWindow);
    AboutBox_Run(hMainWnd);
}

void PanitentApp_RegisterCommands(PanitentApp* pPanitentApp)
{
    AppCmd* pAppCmd = &pPanitentApp->m_appCmd;

    AppCmd_AddCommand(pAppCmd, IDM_FILE_NEW, &PanitentApp_CmdNewFile);
    AppCmd_AddCommand(pAppCmd, IDM_FILE_OPEN, &PanitentApp_CmdOpenFile);
    AppCmd_AddCommand(pAppCmd, IDM_FILE_SAVE, &PanitentApp_CmdSaveFile);
    AppCmd_AddCommand(pAppCmd, IDM_FILE_CLIPBOARD_EXPORT, &PanitentApp_CmdClipboardExport);
    AppCmd_AddCommand(pAppCmd, IDM_FILE_BINVIEW, &PanitentApp_CmdBinView);
    AppCmd_AddCommand(pAppCmd, IDM_FILE_RUN_SCRIPT, &PanitentApp_CmdRunScript);
    AppCmd_AddCommand(pAppCmd, IDM_FILE_CLOSE, &PanitentApp_CmdClose);
    AppCmd_AddCommand(pAppCmd, IDM_EDIT_UNDO, &PanitentApp_CmdUndo);
    AppCmd_AddCommand(pAppCmd, IDM_EDIT_REDO, &PanitentApp_CmdRedo);
    AppCmd_AddCommand(pAppCmd, IDM_EDIT_CLRCANVAS, &PanitentApp_CmdClearCanvas);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_LATLONG_FRONT, &PanitentApp_CmdFilterSpherizeLatLongFront);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_LATLONG_FRONT, &PanitentApp_CmdFilterUnspherizeLatLongFront);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_LATLONG, &PanitentApp_CmdFilterSpherizeLatLong);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_LATLONG, &PanitentApp_CmdFilterUnspherizeLatLong);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_ORTHOGRAPHIC, &PanitentApp_CmdFilterSpherizeOrthographic);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_ORTHOGRAPHIC, &PanitentApp_CmdFilterUnspherizeOrthographic);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_EQUIDISTANT, &PanitentApp_CmdFilterSpherizeEquidistant);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_EQUIDISTANT, &PanitentApp_CmdFilterUnspherizeEquidistant);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_EQUISOLID, &PanitentApp_CmdFilterSpherizeEquisolid);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_EQUISOLID, &PanitentApp_CmdFilterUnspherizeEquisolid);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_STEREOGRAPHIC, &PanitentApp_CmdFilterSpherizeStereographic);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_STEREOGRAPHIC, &PanitentApp_CmdFilterUnspherizeStereographic);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_SPHERIZE_SQUARE_BULGE, &PanitentApp_CmdFilterSpherizeSquareBulge);
    AppCmd_AddCommand(pAppCmd, IDM_FILTER_UNSPHERIZE_SQUARE_BULGE, &PanitentApp_CmdFilterUnspherizeSquareBulge);
    AppCmd_AddCommand(pAppCmd, IDM_WINDOW_ACTIVITY_DIALOG, &PanitentApp_CmdShowActivityDialog);
    AppCmd_AddCommand(pAppCmd, IDM_WINDOW_PROPERTY_GRID, &PanitentApp_CmdShowPropertyGridDialog);
    AppCmd_AddCommand(pAppCmd, IDM_OPTIONS_SETTINGS, &PanitentApp_CmdShowSettings);
    AppCmd_AddCommand(pAppCmd, IDM_HELP_LOG, &PanitentApp_CmdShowLog);
    AppCmd_AddCommand(pAppCmd, IDM_HELP_RBTREEVIZ, &PanitentApp_CmdShowRbTreeViz);
    AppCmd_AddCommand(pAppCmd, IDM_HELP_ABOUT, &PanitentApp_CmdAbout);
    AppCmd_AddCommand(pAppCmd, IDM_HELP_DISPLAYPIXELBUFFER, &PanitentApp_CmdDisplayPixelBuffer);
}

static float PanitentApp_FilterClampSigned1(float value)
{
    if (value < -1.0f)
    {
        return -1.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

static float PanitentApp_FilterClamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

static float PanitentApp_FilterLerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static BOOL PanitentApp_FilterLatLongToDirection(float x, float y, PanitentFilterVector3* pDirection)
{
    const float pi = 3.14159265359f;
    const float halfPi = 1.57079632679f;
    float longitude = x * pi;
    float latitude = -y * halfPi;
    float cosLatitude = cosf(latitude);

    if (!pDirection)
    {
        return FALSE;
    }

    pDirection->x = cosLatitude * sinf(longitude);
    pDirection->y = sinf(latitude);
    pDirection->z = cosLatitude * cosf(longitude);
    return TRUE;
}

static void PanitentApp_FilterDirectionToLatLong(const PanitentFilterVector3* pDirection, float* x, float* y)
{
    const float pi = 3.14159265359f;
    const float halfPi = 1.57079632679f;
    float longitude = 0.0f;
    float latitude = 0.0f;

    if (!pDirection)
    {
        return;
    }

    longitude = atan2f(pDirection->x, pDirection->z);
    latitude = asinf(PanitentApp_FilterClampSigned1(pDirection->y));
    if (x)
    {
        *x = longitude / pi;
    }
    if (y)
    {
        *y = -latitude / halfPi;
    }
}

static BOOL PanitentApp_FilterFrontLatLongToDirection(float x, float y, PanitentFilterVector3* pDirection)
{
    const float halfPi = 1.57079632679f;
    float longitude = x * halfPi;
    float latitude = -y * halfPi;
    float cosLatitude = cosf(latitude);

    if (!pDirection)
    {
        return FALSE;
    }

    pDirection->x = cosLatitude * sinf(longitude);
    pDirection->y = sinf(latitude);
    pDirection->z = cosLatitude * cosf(longitude);
    return TRUE;
}

static BOOL PanitentApp_FilterDirectionToFrontLatLong(const PanitentFilterVector3* pDirection, float* x, float* y)
{
    const float halfPi = 1.57079632679f;
    float longitude = 0.0f;
    float latitude = 0.0f;

    if (!pDirection || pDirection->z < 0.0f)
    {
        return FALSE;
    }

    longitude = atan2f(pDirection->x, max(0.0f, pDirection->z));
    latitude = asinf(PanitentApp_FilterClampSigned1(pDirection->y));
    if (x)
    {
        *x = longitude / halfPi;
    }
    if (y)
    {
        *y = -latitude / halfPi;
    }
    return TRUE;
}

static BOOL PanitentApp_FilterMirrorBallToDirection(float u, float v, PanitentFilterVector3* pDirection)
{
    float diskU = u;
    float diskV = -v;
    float radiusSq = diskU * diskU + diskV * diskV;
    float z = 0.0f;

    if (!pDirection || radiusSq > 1.0f)
    {
        return FALSE;
    }

    z = sqrtf(max(0.0f, 1.0f - radiusSq));
    pDirection->x = 2.0f * diskU * z;
    pDirection->y = 2.0f * diskV * z;
    pDirection->z = 2.0f * z * z - 1.0f;
    return TRUE;
}

static void PanitentApp_FilterDirectionToMirrorBall(const PanitentFilterVector3* pDirection, float hintX, float hintY, float* u, float* v)
{
    float denom = 0.0f;
    float diskU = 0.0f;
    float diskV = 0.0f;

    if (!pDirection)
    {
        return;
    }

    denom = sqrtf(
        pDirection->x * pDirection->x +
        pDirection->y * pDirection->y +
        (pDirection->z + 1.0f) * (pDirection->z + 1.0f));

    if (denom > 1.0e-6f)
    {
        diskU = pDirection->x / denom;
        diskV = pDirection->y / denom;
    }
    else
    {
        float hintRadius = sqrtf(hintX * hintX + hintY * hintY);
        if (hintRadius > 1.0e-6f)
        {
            diskU = hintX / hintRadius;
            diskV = -hintY / hintRadius;
        }
        else
        {
            diskU = 1.0f;
            diskV = 0.0f;
        }
    }

    if (u)
    {
        *u = diskU;
    }
    if (v)
    {
        *v = -diskV;
    }
}

static void PanitentApp_FilterSquareToDisk(float x, float y, float* u, float* v)
{
    float uu = x * sqrtf(max(0.0f, 1.0f - (y * y) * 0.5f));
    float vv = y * sqrtf(max(0.0f, 1.0f - (x * x) * 0.5f));

    if (u)
    {
        *u = uu;
    }
    if (v)
    {
        *v = vv;
    }
}

static BOOL PanitentApp_FilterDiskToSquare(float u, float v, float* x, float* y)
{
    float radiusSq = u * u + v * v;
    float termX = 2.0f + u * u - v * v;
    float termY = 2.0f - u * u + v * v;
    float sqrt2u = 2.0f * 1.41421356237f * u;
    float sqrt2v = 2.0f * 1.41421356237f * v;
    float xValue = 0.0f;
    float yValue = 0.0f;

    if (radiusSq > 1.0f)
    {
        return FALSE;
    }

    xValue = 0.5f * (sqrtf(max(0.0f, termX + sqrt2u)) - sqrtf(max(0.0f, termX - sqrt2u)));
    yValue = 0.5f * (sqrtf(max(0.0f, termY + sqrt2v)) - sqrtf(max(0.0f, termY - sqrt2v)));
    if (x)
    {
        *x = xValue;
    }
    if (y)
    {
        *y = yValue;
    }
    return TRUE;
}

static void PanitentApp_FilterSquareBulgeToDisk(float x, float y, float* u, float* v)
{
    float radius = max(fabsf(x), fabsf(y));
    float angle = 0.0f;
    float uu = 0.0f;
    float vv = 0.0f;

    if (radius > 1.0e-6f)
    {
        angle = atan2f(y, x);
        uu = cosf(angle) * radius;
        vv = sinf(angle) * radius;
    }

    if (u)
    {
        *u = uu;
    }
    if (v)
    {
        *v = vv;
    }
}

static BOOL PanitentApp_FilterDiskToSquareBulge(float u, float v, float* x, float* y)
{
    float radiusSq = u * u + v * v;
    float diskRadius = 0.0f;
    float angle = 0.0f;
    float dx = 0.0f;
    float dy = 0.0f;
    float scale = 0.0f;
    float xValue = 0.0f;
    float yValue = 0.0f;

    if (radiusSq > 1.0f)
    {
        return FALSE;
    }

    diskRadius = sqrtf(radiusSq);
    if (diskRadius > 1.0e-6f)
    {
        angle = atan2f(v, u);
        dx = cosf(angle);
        dy = sinf(angle);
        scale = max(fabsf(dx), fabsf(dy));
        if (scale > 1.0e-6f)
        {
            xValue = (dx / scale) * diskRadius;
            yValue = (dy / scale) * diskRadius;
        }
    }

    if (x)
    {
        *x = xValue;
    }
    if (y)
    {
        *y = yValue;
    }
    return TRUE;
}

static float PanitentApp_FilterProjectRadius(PanitentFilterProjection projection, float radius)
{
    const float halfPi = 1.57079632679f;
    const float root2 = 1.41421356237f;

    radius = PanitentApp_FilterClamp01(radius);
    switch (projection)
    {
    case PanitentFilterProjection_LatLong:
    case PanitentFilterProjection_LatLongFront:
        return radius;
    case PanitentFilterProjection_Orthographic:
    case PanitentFilterProjection_SquareBulge:
        return sinf(radius * halfPi);
    case PanitentFilterProjection_Equidistant:
        return radius;
    case PanitentFilterProjection_Equisolid:
        return root2 * sinf(radius * 0.78539816339f);
    case PanitentFilterProjection_Stereographic:
        return tanf(radius * 0.78539816339f);
    default:
        return radius;
    }
}

static float PanitentApp_FilterUnprojectRadius(PanitentFilterProjection projection, float radius)
{
    const float halfPi = 1.57079632679f;
    const float root2 = 1.41421356237f;

    radius = PanitentApp_FilterClamp01(radius);
    switch (projection)
    {
    case PanitentFilterProjection_LatLong:
    case PanitentFilterProjection_LatLongFront:
        return radius;
    case PanitentFilterProjection_Orthographic:
    case PanitentFilterProjection_SquareBulge:
        return asinf(radius) / halfPi;
    case PanitentFilterProjection_Equidistant:
        return radius;
    case PanitentFilterProjection_Equisolid:
        return (4.0f * asinf(min(1.0f, radius / root2))) / 3.14159265359f;
    case PanitentFilterProjection_Stereographic:
        return (4.0f * atanf(radius)) / 3.14159265359f;
    default:
        return radius;
    }
}

static uint32_t PanitentApp_FilterSampleBilinear(const Canvas* pCanvas, float x, float y)
{
    const uint32_t* pBuffer = NULL;
    int width = 0;
    int height = 0;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    float tx = 0.0f;
    float ty = 0.0f;
    float a = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    uint32_t p00 = 0;
    uint32_t p10 = 0;
    uint32_t p01 = 0;
    uint32_t p11 = 0;

    if (!pCanvas || !pCanvas->buffer || pCanvas->width <= 0 || pCanvas->height <= 0)
    {
        return 0;
    }

    width = pCanvas->width;
    height = pCanvas->height;
    x = max(0.0f, min((float)(width - 1), x));
    y = max(0.0f, min((float)(height - 1), y));
    x0 = (int)floorf(x);
    y0 = (int)floorf(y);
    x1 = min(width - 1, x0 + 1);
    y1 = min(height - 1, y0 + 1);
    tx = x - (float)x0;
    ty = y - (float)y0;

    pBuffer = (const uint32_t*)pCanvas->buffer;
    p00 = pBuffer[(size_t)y0 * (size_t)width + (size_t)x0];
    p10 = pBuffer[(size_t)y0 * (size_t)width + (size_t)x1];
    p01 = pBuffer[(size_t)y1 * (size_t)width + (size_t)x0];
    p11 = pBuffer[(size_t)y1 * (size_t)width + (size_t)x1];

    a = PanitentApp_FilterLerp(
        PanitentApp_FilterLerp((float)CHANNEL_A_32(p00), (float)CHANNEL_A_32(p10), tx),
        PanitentApp_FilterLerp((float)CHANNEL_A_32(p01), (float)CHANNEL_A_32(p11), tx),
        ty);
    r = PanitentApp_FilterLerp(
        PanitentApp_FilterLerp((float)CHANNEL_R_32(p00), (float)CHANNEL_R_32(p10), tx),
        PanitentApp_FilterLerp((float)CHANNEL_R_32(p01), (float)CHANNEL_R_32(p11), tx),
        ty);
    g = PanitentApp_FilterLerp(
        PanitentApp_FilterLerp((float)CHANNEL_G_32(p00), (float)CHANNEL_G_32(p10), tx),
        PanitentApp_FilterLerp((float)CHANNEL_G_32(p01), (float)CHANNEL_G_32(p11), tx),
        ty);
    b = PanitentApp_FilterLerp(
        PanitentApp_FilterLerp((float)CHANNEL_B_32(p00), (float)CHANNEL_B_32(p10), tx),
        PanitentApp_FilterLerp((float)CHANNEL_B_32(p01), (float)CHANNEL_B_32(p11), tx),
        ty);

    return ARGB((uint8_t)(a + 0.5f), (uint8_t)(r + 0.5f), (uint8_t)(g + 0.5f), (uint8_t)(b + 0.5f));
}

static void PanitentApp_ApplySpherizeFilter(PanitentApp* pPanitentApp, BOOL fUnspherize, PanitentFilterProjection projection)
{
    Document* pDocument = NULL;
    Canvas* pCanvas = NULL;
    Canvas* pResult = NULL;
    int width = 0;
    int height = 0;

    if (!pPanitentApp)
    {
        return;
    }

    pDocument = PanitentApp_GetActiveDocument(pPanitentApp);
    if (!pDocument)
    {
        return;
    }

    pCanvas = Document_GetCanvas(pDocument);
    if (!pCanvas || !pCanvas->buffer || pCanvas->width <= 0 || pCanvas->height <= 0)
    {
        return;
    }

    width = pCanvas->width;
    height = pCanvas->height;
    pResult = Canvas_Create(width, height);
    if (!pResult)
    {
        return;
    }

    memset(pResult->buffer, 0, pResult->buffer_size);
    History_StartDifferentiation(pDocument);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float nx = ((2.0f * ((float)x + 0.5f)) / (float)width) - 1.0f;
            float ny = ((2.0f * ((float)y + 0.5f)) / (float)height) - 1.0f;
            float sx = 0.0f;
            float sy = 0.0f;
            float dx = 0.0f;
            float dy = 0.0f;
            uint32_t color = 0;

            if (fUnspherize)
            {
                float radius = 0.0f;

                if (projection == PanitentFilterProjection_LatLong || projection == PanitentFilterProjection_LatLongFront)
                {
                    PanitentFilterVector3 direction;
                    BOOL fOk = FALSE;

                    if (projection == PanitentFilterProjection_LatLongFront)
                    {
                        fOk = PanitentApp_FilterFrontLatLongToDirection(nx, ny, &direction);
                    }
                    else
                    {
                        fOk = PanitentApp_FilterLatLongToDirection(nx, ny, &direction);
                    }

                    if (!fOk)
                    {
                        ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = 0;
                        continue;
                    }

                    PanitentApp_FilterDirectionToMirrorBall(&direction, nx, ny, &dx, &dy);
                    sx = ((PanitentApp_FilterClamp01((dx + 1.0f) * 0.5f)) * (float)(width - 1));
                    sy = ((PanitentApp_FilterClamp01((dy + 1.0f) * 0.5f)) * (float)(height - 1));
                    color = PanitentApp_FilterSampleBilinear(pCanvas, sx, sy);
                    ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = color;
                    continue;
                }
                else if (projection == PanitentFilterProjection_SquareBulge)
                {
                    PanitentApp_FilterSquareBulgeToDisk(nx, ny, &dx, &dy);
                }
                else
                {
                    PanitentApp_FilterSquareToDisk(nx, ny, &dx, &dy);
                }
                radius = sqrtf(dx * dx + dy * dy);
                if (radius > 1.0e-6f)
                {
                    float bulgedRadius = PanitentApp_FilterProjectRadius(projection, radius);
                    float scale = bulgedRadius / radius;
                    dx *= scale;
                    dy *= scale;
                }
                sx = ((dx + 1.0f) * 0.5f) * (float)(width - 1);
                sy = ((dy + 1.0f) * 0.5f) * (float)(height - 1);
                color = PanitentApp_FilterSampleBilinear(pCanvas, sx, sy);
            }
            else {
                float radius = sqrtf(nx * nx + ny * ny);

                if (radius > 1.0f)
                {
                    color = 0;
                }
                else {
                    float unbulgedRadius = PanitentApp_FilterUnprojectRadius(projection, radius);
                    float su = 0.0f;
                    float sv = 0.0f;
                    BOOL fOk = FALSE;

                    if (radius > 1.0e-6f)
                    {
                        float scale = unbulgedRadius / radius;
                        su = nx * scale;
                        sv = ny * scale;
                    }

                    if (projection == PanitentFilterProjection_LatLong || projection == PanitentFilterProjection_LatLongFront)
                    {
                        PanitentFilterVector3 direction;
                        BOOL fMapOk = FALSE;

                        if (!PanitentApp_FilterMirrorBallToDirection(nx, ny, &direction))
                        {
                            color = 0;
                            ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = color;
                            continue;
                        }

                        if (projection == PanitentFilterProjection_LatLongFront)
                        {
                            fMapOk = PanitentApp_FilterDirectionToFrontLatLong(&direction, &dx, &dy);
                        }
                        else
                        {
                            PanitentApp_FilterDirectionToLatLong(&direction, &dx, &dy);
                            fMapOk = TRUE;
                        }

                        if (!fMapOk)
                        {
                            color = 0;
                            ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = color;
                            continue;
                        }
                        sx = ((PanitentApp_FilterClamp01((dx + 1.0f) * 0.5f)) * (float)(width - 1));
                        sy = ((PanitentApp_FilterClamp01((dy + 1.0f) * 0.5f)) * (float)(height - 1));
                        color = PanitentApp_FilterSampleBilinear(pCanvas, sx, sy);
                        ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = color;
                        continue;
                    }
                    else if (projection == PanitentFilterProjection_SquareBulge)
                    {
                        fOk = PanitentApp_FilterDiskToSquareBulge(su, sv, &dx, &dy);
                    }
                    else
                    {
                        fOk = PanitentApp_FilterDiskToSquare(su, sv, &dx, &dy);
                    }

                    if (projection != PanitentFilterProjection_LatLong &&
                        projection != PanitentFilterProjection_LatLongFront &&
                        !fOk)
                    {
                        color = 0;
                        ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = color;
                        continue;
                    }
                    sx = ((PanitentApp_FilterClamp01((dx + 1.0f) * 0.5f)) * (float)(width - 1));
                    sy = ((PanitentApp_FilterClamp01((dy + 1.0f) * 0.5f)) * (float)(height - 1));
                    color = PanitentApp_FilterSampleBilinear(pCanvas, sx, sy);
                }
            }

            ((uint32_t*)pResult->buffer)[(size_t)y * (size_t)width + (size_t)x] = color;
        }
    }

    memcpy(pCanvas->buffer, pResult->buffer, pCanvas->buffer_size);
    History_FinalizeDifferentiation(pDocument);
    Window_Invalidate(PanitentApp_GetActiveViewport(pPanitentApp));

    Canvas_Delete(pResult);
    free(pResult);
}

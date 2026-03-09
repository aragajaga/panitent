#include "precomp.h"

#include <stdint.h>

#include "windowlayoutmanager.h"

#include "dockfloatingpersist.h"
#include "dockfloatingmodel.h"
#include "floatingdocumentlayoutmodel.h"
#include "floatingdocumentlayoutpersist.h"
#include "dockhost.h"
#include "dockhostrestore.h"
#include "docklayoutpersist.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockmodelio.h"
#include "dockmodelvalidate.h"
#include "dockshell.h"
#include "dockviewfactory.h"
#include "floatingwindowcontainer.h"
#include "panitentapp.h"
#include "panitentwindow.h"
#include "persistfile.h"
#include "resource.h"
#include "shell/pathutil.h"
#include "windowlayoutcatalog.h"
#include "win32/window.h"
#include "win32/windowmap.h"
#include "workspacecontainer.h"

#define WINDOW_LAYOUT_MAX_MENU_ITEMS (IDM_WINDOW_APPLY_LAYOUT_LAST - IDM_WINDOW_APPLY_LAYOUT_BASE + 1)

static const WCHAR g_szWindowLayoutsFileName[] = L"windowlayouts.dat";

typedef struct WindowLayoutNameDialogContext
{
    WCHAR szTitle[64];
    WCHAR szPrompt[128];
    WCHAR szName[WINDOW_LAYOUT_NAME_MAX];
} WindowLayoutNameDialogContext;

typedef struct WindowLayoutManageDialogContext
{
    PanitentWindow* pPanitentWindow;
    WindowLayoutCatalog catalog;
} WindowLayoutManageDialogContext;

typedef struct WindowLayoutApplyContext
{
    WorkspaceContainer* pWorkspaceContainer;
} WindowLayoutApplyContext;

static HMENU WindowLayoutManager_GetWindowMenu(PanitentWindow* pPanitentWindow);
static HMENU WindowLayoutManager_GetApplyMenu(PanitentWindow* pPanitentWindow);
static BOOL WindowLayoutManager_GetCatalogPath(PTSTR* ppszPath);
static BOOL WindowLayoutManager_GetDockLayoutPath(uint32_t uId, PTSTR* ppszPath);
static BOOL WindowLayoutManager_GetDockFloatingPath(uint32_t uId, PTSTR* ppszPath);
static BOOL WindowLayoutManager_GetFloatDocLayoutPath(uint32_t uId, PTSTR* ppszPath);
static BOOL WindowLayoutManager_LoadCatalog(WindowLayoutCatalog* pCatalog);
static BOOL WindowLayoutManager_SaveCatalog(const WindowLayoutCatalog* pCatalog);
static BOOL WindowLayoutManager_SaveProfile(PanitentWindow* pPanitentWindow, uint32_t uId);
static BOOL WindowLayoutManager_ApplyProfile(PanitentWindow* pPanitentWindow, uint32_t uId);
static BOOL WindowLayoutManager_ApplyDefault(PanitentWindow* pPanitentWindow);
static BOOL WindowLayoutManager_ApplyModel(PanitentWindow* pPanitentWindow, DockModelNode* pModelRoot, const DockFloatingLayoutFileModel* pFloatingModel, const FloatingDocumentLayoutModel* pFloatDocModel);
static BOOL WindowLayoutManager_ApplyTransactional(PanitentWindow* pPanitentWindow, DockModelNode* pTargetModelRoot, const DockFloatingLayoutFileModel* pTargetFloatingModel, const FloatingDocumentLayoutModel* pTargetFloatDocModel);
static BOOL WindowLayoutManager_DestroyFloatingToolWindows(void);
static void WindowLayoutManager_FillListBox(HWND hWndList, const WindowLayoutCatalog* pCatalog);
static BOOL WindowLayoutManager_PromptForName(HWND hWndParent, PCWSTR pszTitle, PCWSTR pszPrompt, PCWSTR pszInitialName, WCHAR* pszName, size_t cchName);
static INT_PTR CALLBACK WindowLayoutManager_NameDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK WindowLayoutManager_ManageDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static Window* WindowLayoutManager_ResolveView(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    PanitentDockViewId nViewId,
    void* pUserData);

static HMENU WindowLayoutManager_GetWindowMenu(PanitentWindow* pPanitentWindow)
{
    if (!pPanitentWindow || !pPanitentWindow->hMainMenu)
    {
        return NULL;
    }

    return GetSubMenu(pPanitentWindow->hMainMenu, 4);
}

static HMENU WindowLayoutManager_GetApplyMenu(PanitentWindow* pPanitentWindow)
{
    HMENU hWindowMenu = WindowLayoutManager_GetWindowMenu(pPanitentWindow);
    if (!hWindowMenu)
    {
        return NULL;
    }

    int nItems = GetMenuItemCount(hWindowMenu);
    for (int i = 0; i < nItems; ++i)
    {
        HMENU hSubMenu = GetSubMenu(hWindowMenu, i);
        if (!hSubMenu)
        {
            continue;
        }
        if (GetMenuItemCount(hSubMenu) > 0 && GetMenuItemID(hSubMenu, 0) == IDM_WINDOW_APPLY_LAYOUT_NONE)
        {
            return hSubMenu;
        }
    }

    return NULL;
}

static BOOL WindowLayoutManager_GetCatalogPath(PTSTR* ppszPath)
{
    GetAppDataFilePath(g_szWindowLayoutsFileName, ppszPath);
    return ppszPath && *ppszPath;
}

static BOOL WindowLayoutManager_GetDockLayoutPath(uint32_t uId, PTSTR* ppszPath)
{
    WCHAR szRelative[MAX_PATH] = L"";
    StringCchPrintfW(szRelative, ARRAYSIZE(szRelative), L"windowlayout_%08X_docklayout.dat", uId);
    GetAppDataFilePath(szRelative, ppszPath);
    return ppszPath && *ppszPath;
}

static BOOL WindowLayoutManager_GetDockFloatingPath(uint32_t uId, PTSTR* ppszPath)
{
    WCHAR szRelative[MAX_PATH] = L"";
    StringCchPrintfW(szRelative, ARRAYSIZE(szRelative), L"windowlayout_%08X_dockfloating.dat", uId);
    GetAppDataFilePath(szRelative, ppszPath);
    return ppszPath && *ppszPath;
}

static BOOL WindowLayoutManager_GetFloatDocLayoutPath(uint32_t uId, PTSTR* ppszPath)
{
    WCHAR szRelative[MAX_PATH] = L"";
    StringCchPrintfW(szRelative, ARRAYSIZE(szRelative), L"windowlayout_%08X_floatdoclayout.dat", uId);
    GetAppDataFilePath(szRelative, ppszPath);
    return ppszPath && *ppszPath;
}

static BOOL WindowLayoutManager_LoadCatalog(WindowLayoutCatalog* pCatalog)
{
    PTSTR pszPath = NULL;
    PersistLoadStatus status = PERSIST_LOAD_NOT_FOUND;
    if (!pCatalog)
    {
        return FALSE;
    }

    WindowLayoutCatalog_Init(pCatalog);
    if (!WindowLayoutManager_GetCatalogPath(&pszPath))
    {
        return FALSE;
    }

    BOOL bLoaded = WindowLayoutCatalog_LoadFromFile(pszPath, pCatalog, &status);
    if (!bLoaded && status == PERSIST_LOAD_INVALID_FORMAT)
    {
        PersistFile_QuarantineInvalid(pszPath);
    }
    free(pszPath);
    return bLoaded || status == PERSIST_LOAD_NOT_FOUND;
}

static BOOL WindowLayoutManager_SaveCatalog(const WindowLayoutCatalog* pCatalog)
{
    PTSTR pszPath = NULL;
    if (!pCatalog || !WindowLayoutManager_GetCatalogPath(&pszPath))
    {
        return FALSE;
    }

    BOOL bSaved = WindowLayoutCatalog_SaveToFile(pCatalog, pszPath);
    free(pszPath);
    return bSaved;
}

void WindowLayoutManager_RefreshApplyMenu(PanitentWindow* pPanitentWindow)
{
    HMENU hApplyMenu = WindowLayoutManager_GetApplyMenu(pPanitentWindow);
    if (!hApplyMenu)
    {
        return;
    }

    while (GetMenuItemCount(hApplyMenu) > 0)
    {
        DeleteMenu(hApplyMenu, 0, MF_BYPOSITION);
    }

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutManager_LoadCatalog(&catalog);
    if (catalog.nEntryCount <= 0)
    {
        AppendMenuW(hApplyMenu, MF_STRING | MF_GRAYED, IDM_WINDOW_APPLY_LAYOUT_NONE, L"(No saved layouts)");
    }
    else {
        int nItems = min(catalog.nEntryCount, WINDOW_LAYOUT_MAX_MENU_ITEMS);
        for (int i = 0; i < nItems; ++i)
        {
            AppendMenuW(hApplyMenu, MF_STRING, IDM_WINDOW_APPLY_LAYOUT_BASE + i, catalog.entries[i].szName);
        }
    }

    HWND hWnd = Window_GetHWND((Window*)pPanitentWindow);
    if (hWnd && IsWindow(hWnd))
    {
        DrawMenuBar(hWnd);
    }
}

static BOOL CALLBACK WindowLayoutManager_DestroyFloatingToolEnumProc(HWND hWnd, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    WCHAR szClassName[64] = L"";
    if (!IsWindow(hWnd))
    {
        return TRUE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    if (wcscmp(szClassName, L"__FloatingWindowContainer") != 0)
    {
        return TRUE;
    }

    FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)WindowMap_Get(hWnd);
    if (pFloatingWindowContainer && pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_PANEL)
    {
        DestroyWindow(hWnd);
    }

    return TRUE;
}

static BOOL WindowLayoutManager_DestroyFloatingToolWindows(void)
{
    return EnumWindows(WindowLayoutManager_DestroyFloatingToolEnumProc, 0);
}

static Window* WindowLayoutManager_ResolveView(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    PanitentDockViewId nViewId,
    void* pUserData)
{
    UNREFERENCED_PARAMETER(pDockHostWindow);
    UNREFERENCED_PARAMETER(pNode);
    UNREFERENCED_PARAMETER(pDockData);

    WindowLayoutApplyContext* pContext = (WindowLayoutApplyContext*)pUserData;
    if (nViewId == PNT_DOCK_VIEW_WORKSPACE && pContext && pContext->pWorkspaceContainer)
    {
        if (!pPanitentApp->m_pWorkspaceContainer)
        {
            pPanitentApp->m_pWorkspaceContainer = pContext->pWorkspaceContainer;
        }
        return (Window*)pContext->pWorkspaceContainer;
    }

    return NULL;
}

static BOOL WindowLayoutManager_ApplyModel(PanitentWindow* pPanitentWindow, DockModelNode* pModelRoot, const DockFloatingLayoutFileModel* pFloatingModel, const FloatingDocumentLayoutModel* pFloatDocModel)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    DockHostWindow* pDockHostWindow = pPanitentWindow ? pPanitentWindow->m_pDockHostWindow : NULL;
    WorkspaceContainer* pWorkspaceContainer = pPanitentApp ? pPanitentApp->m_pWorkspaceContainer : NULL;
    HWND hWndWorkspace = pWorkspaceContainer ? Window_GetHWND((Window*)pWorkspaceContainer) : NULL;
    RECT rcDockHost = { 0 };
    TreeNode* pRootNode;

    if (!pPanitentWindow || !pPanitentApp || !pDockHostWindow || !pModelRoot)
    {
        return FALSE;
    }

    pRootNode = DockModelBuildTree(pModelRoot);
    if (!pRootNode || !pRootNode->data)
    {
        DockModelBuildDestroyTree(pRootNode);
        return FALSE;
    }

    if (Window_IsWindow((Window*)pDockHostWindow))
    {
        Window_GetClientRect((Window*)pDockHostWindow, &rcDockHost);
    }
    ((DockData*)pRootNode->data)->rc = rcDockHost;

    WindowLayoutManager_DestroyFloatingToolWindows();
    if (hWndWorkspace && IsWindow(hWndWorkspace))
    {
        DockHostWindow_ClearLayout(pDockHostWindow, &hWndWorkspace, 1);
    }
    else {
        DockHostWindow_ClearLayout(pDockHostWindow, NULL, 0);
    }

    pPanitentApp->m_pPaletteWindow = NULL;
    PanitentApp_SetOptionBar(pPanitentApp, NULL);

    WindowLayoutApplyContext context = { 0 };
    context.pWorkspaceContainer = pWorkspaceContainer;
    BOOL bHasWorkspace = FALSE;
    if (!PanitentDockHostRestoreAttachKnownViewsEx(
        pPanitentApp,
        pDockHostWindow,
        pRootNode,
        WindowLayoutManager_ResolveView,
        &context,
        NULL,
        NULL,
        &bHasWorkspace))
    {
        if (hWndWorkspace && IsWindow(hWndWorkspace))
        {
            DockHostWindow_DestroyNodeTree(pRootNode, &hWndWorkspace, 1);
        }
        else {
            DockHostWindow_DestroyNodeTree(pRootNode, NULL, 0);
        }
        return FALSE;
    }

    DockHostWindow_SetRoot(pDockHostWindow, pRootNode);
    DockHostWindow_Rearrange(pDockHostWindow);

    if (pFloatingModel && !PanitentDockFloating_RestoreModel(pPanitentApp, pDockHostWindow, pFloatingModel))
    {
        return FALSE;
    }

    if (pFloatDocModel && !PanitentFloatingDocumentLayout_RestoreModel(pPanitentApp, pDockHostWindow, pFloatDocModel))
    {
        return FALSE;
    }

    return bHasWorkspace && pPanitentApp->m_pWorkspaceContainer != NULL;
}

static BOOL WindowLayoutManager_ApplyTransactional(PanitentWindow* pPanitentWindow, DockModelNode* pTargetModelRoot, const DockFloatingLayoutFileModel* pTargetFloatingModel, const FloatingDocumentLayoutModel* pTargetFloatDocModel)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    DockHostWindow* pDockHostWindow = pPanitentWindow ? pPanitentWindow->m_pDockHostWindow : NULL;
    DockModelNode* pRollbackModel = NULL;
    DockFloatingLayoutFileModel rollbackFloating = { 0 };
    FloatingDocumentLayoutModel rollbackFloatDocs = { 0 };
    BOOL bHaveRollbackFloating = FALSE;
    BOOL bHaveRollbackFloatDocs = FALSE;
    BOOL bApplied = FALSE;

    if (!pPanitentWindow || !pPanitentApp || !pDockHostWindow || !pTargetModelRoot)
    {
        return FALSE;
    }

    pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    if (!pRollbackModel)
    {
        return FALSE;
    }
    if (!PanitentDockFloating_CaptureModel(pPanitentApp, pDockHostWindow, &rollbackFloating))
    {
        DockModel_Destroy(pRollbackModel);
        return FALSE;
    }
    bHaveRollbackFloating = TRUE;
    if (!PanitentFloatingDocumentLayout_CaptureModel(pPanitentApp, &rollbackFloatDocs))
    {
        DockModel_Destroy(pRollbackModel);
        DockFloatingLayout_Destroy(&rollbackFloating);
        return FALSE;
    }
    bHaveRollbackFloatDocs = TRUE;

    bApplied = WindowLayoutManager_ApplyModel(pPanitentWindow, pTargetModelRoot, pTargetFloatingModel, pTargetFloatDocModel);
    if (!bApplied)
    {
        WindowLayoutManager_DestroyFloatingToolWindows();
        WindowLayoutManager_ApplyModel(
            pPanitentWindow,
            pRollbackModel,
            bHaveRollbackFloating ? &rollbackFloating : NULL,
            bHaveRollbackFloatDocs ? &rollbackFloatDocs : NULL);
    }

    DockModel_Destroy(pRollbackModel);
    if (bHaveRollbackFloating)
    {
        DockFloatingLayout_Destroy(&rollbackFloating);
    }
    if (bHaveRollbackFloatDocs)
    {
        FloatingDocumentLayoutModel_Destroy(&rollbackFloatDocs);
    }
    return bApplied;
}

static BOOL WindowLayoutManager_SaveProfile(PanitentWindow* pPanitentWindow, uint32_t uId)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    PTSTR pszDockLayoutPath = NULL;
    PTSTR pszDockFloatingPath = NULL;
    PTSTR pszFloatDocLayoutPath = NULL;
    FloatingDocumentLayoutModel floatDocModel = { 0 };
    BOOL bLayoutSaved;
    BOOL bFloatingSaved;
    BOOL bFloatDocsSaved;

    if (!pPanitentWindow || !pPanitentWindow->m_pDockHostWindow ||
        !WindowLayoutManager_GetDockLayoutPath(uId, &pszDockLayoutPath) ||
        !WindowLayoutManager_GetDockFloatingPath(uId, &pszDockFloatingPath) ||
        !WindowLayoutManager_GetFloatDocLayoutPath(uId, &pszFloatDocLayoutPath))
    {
        free(pszDockLayoutPath);
        free(pszDockFloatingPath);
        free(pszFloatDocLayoutPath);
        return FALSE;
    }

    bLayoutSaved = PanitentDockLayout_SaveToFilePath(pPanitentApp, pPanitentWindow->m_pDockHostWindow, pszDockLayoutPath);
    bFloatingSaved = PanitentDockFloating_SaveToFilePath(pPanitentApp, pPanitentWindow->m_pDockHostWindow, pszDockFloatingPath);
    bFloatDocsSaved =
        PanitentFloatingDocumentLayout_CaptureModel(pPanitentApp, &floatDocModel) &&
        FloatingDocumentLayoutModel_SaveToFile(&floatDocModel, pszFloatDocLayoutPath);
    free(pszDockLayoutPath);
    free(pszDockFloatingPath);
    free(pszFloatDocLayoutPath);
    FloatingDocumentLayoutModel_Destroy(&floatDocModel);
    return bLayoutSaved && bFloatingSaved && bFloatDocsSaved;
}

static BOOL WindowLayoutManager_ApplyProfile(PanitentWindow* pPanitentWindow, uint32_t uId)
{
    PTSTR pszDockLayoutPath = NULL;
    PTSTR pszDockFloatingPath = NULL;
    PTSTR pszFloatDocLayoutPath = NULL;
    DockModelNode* pModelRoot = NULL;
    DockFloatingLayoutFileModel floatingModel = { 0 };
    FloatingDocumentLayoutModel floatDocModel = { 0 };
    PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
    BOOL bResult = FALSE;

    if (!WindowLayoutManager_GetDockLayoutPath(uId, &pszDockLayoutPath) ||
        !WindowLayoutManager_GetDockFloatingPath(uId, &pszDockFloatingPath) ||
        !WindowLayoutManager_GetFloatDocLayoutPath(uId, &pszFloatDocLayoutPath))
    {
        free(pszDockLayoutPath);
        free(pszDockFloatingPath);
        free(pszFloatDocLayoutPath);
        return FALSE;
    }

    pModelRoot = DockModelIO_LoadFromFileEx(pszDockLayoutPath, &loadStatus);
    if (!pModelRoot || !DockModelValidateAndRepairMainLayout(&pModelRoot, NULL))
    {
        DockModel_Destroy(pModelRoot);
        free(pszDockLayoutPath);
        free(pszDockFloatingPath);
        free(pszFloatDocLayoutPath);
        return FALSE;
    }

    loadStatus = PERSIST_LOAD_IO_ERROR;
    if (pszDockFloatingPath && pszDockFloatingPath[0])
    {
        BOOL bLoadedFloating = DockFloatingLayout_LoadFromFileEx(pszDockFloatingPath, &floatingModel, &loadStatus);
        if (!bLoadedFloating && loadStatus != PERSIST_LOAD_NOT_FOUND)
        {
            DockModel_Destroy(pModelRoot);
            free(pszDockLayoutPath);
            free(pszDockFloatingPath);
            free(pszFloatDocLayoutPath);
            return FALSE;
        }
    }

    loadStatus = PERSIST_LOAD_IO_ERROR;
    if (pszFloatDocLayoutPath && pszFloatDocLayoutPath[0])
    {
        BOOL bLoadedFloatDocs = FloatingDocumentLayoutModel_LoadFromFileEx(pszFloatDocLayoutPath, &floatDocModel, &loadStatus);
        if (!bLoadedFloatDocs && loadStatus != PERSIST_LOAD_NOT_FOUND)
        {
            DockModel_Destroy(pModelRoot);
            DockFloatingLayout_Destroy(&floatingModel);
            free(pszDockLayoutPath);
            free(pszDockFloatingPath);
            free(pszFloatDocLayoutPath);
            return FALSE;
        }
    }

    bResult = WindowLayoutManager_ApplyTransactional(pPanitentWindow, pModelRoot, &floatingModel, &floatDocModel);
    DockFloatingLayout_Destroy(&floatingModel);
    FloatingDocumentLayoutModel_Destroy(&floatDocModel);
    DockModel_Destroy(pModelRoot);
    free(pszDockLayoutPath);
    free(pszDockFloatingPath);
    free(pszFloatDocLayoutPath);
    return bResult;
}

static BOOL WindowLayoutManager_ApplyDefault(PanitentWindow* pPanitentWindow)
{
    DockShellMetrics shellMetrics = { 220, 300, 72, 72 };
    TreeNode* pRoot = DockShell_CreateRootNode();
    TreeNode* pWorkspace = PanitentDockViewFactory_CreateNode(DOCK_ROLE_WORKSPACE, L"WorkspaceContainer");
    TreeNode* pZoneLeft = DockShell_CreateZoneNode(DKS_LEFT);
    TreeNode* pZoneRight = DockShell_CreateZoneNode(DKS_RIGHT);
    TreeNode* pZoneTop = DockShell_CreateZoneNode(DKS_TOP);
    TreeNode* pZoneBottom = DockShell_CreateZoneNode(DKS_BOTTOM);
    TreeNode* pToolbox = PanitentDockViewFactory_CreateNode(DOCK_ROLE_PANEL, L"Toolbox");
    TreeNode* pGLWindow = PanitentDockViewFactory_CreateNode(DOCK_ROLE_PANEL, L"GLWindow");
    TreeNode* pPalette = PanitentDockViewFactory_CreateNode(DOCK_ROLE_PANEL, L"Palette");
    TreeNode* pLayers = PanitentDockViewFactory_CreateNode(DOCK_ROLE_PANEL, L"Layers");
    TreeNode* pOptionBar = PanitentDockViewFactory_CreateNode(DOCK_ROLE_PANEL, L"Option Bar");
    DockModelNode* pModelRoot;
    BOOL bResult;

    if (!pRoot || !pWorkspace || !pZoneLeft || !pZoneRight || !pZoneTop || !pZoneBottom ||
        !pToolbox || !pGLWindow || !pPalette || !pLayers || !pOptionBar)
    {
        DockModelBuildDestroyTree(pRoot);
        DockModelBuildDestroyTree(pWorkspace);
        DockModelBuildDestroyTree(pZoneLeft);
        DockModelBuildDestroyTree(pZoneRight);
        DockModelBuildDestroyTree(pZoneTop);
        DockModelBuildDestroyTree(pZoneBottom);
        DockModelBuildDestroyTree(pToolbox);
        DockModelBuildDestroyTree(pGLWindow);
        DockModelBuildDestroyTree(pPalette);
        DockModelBuildDestroyTree(pLayers);
        DockModelBuildDestroyTree(pOptionBar);
        return FALSE;
    }

    DockShell_AppendPanelToZone(pZoneLeft, pToolbox);
    DockShell_AppendPanelToZone(pZoneRight, pGLWindow);
    DockShell_AppendPanelToZone(pZoneRight, pPalette);
    DockShell_AppendPanelToZone(pZoneRight, pLayers);
    DockShell_AppendPanelToZone(pZoneBottom, pOptionBar);
    if (!DockShell_BuildMainLayout(pRoot, pWorkspace, pZoneLeft, pZoneRight, pZoneTop, pZoneBottom, &shellMetrics))
    {
        DockModelBuildDestroyTree(pRoot);
        return FALSE;
    }

    pModelRoot = DockModel_CaptureTree(pRoot);
    DockModelBuildDestroyTree(pRoot);
    if (!pModelRoot)
    {
        return FALSE;
    }

    bResult = WindowLayoutManager_ApplyTransactional(pPanitentWindow, pModelRoot, NULL, NULL);
    DockModel_Destroy(pModelRoot);
    return bResult;
}

static void WindowLayoutManager_FillListBox(HWND hWndList, const WindowLayoutCatalog* pCatalog)
{
    if (!hWndList || !pCatalog)
    {
        return;
    }

    SendMessageW(hWndList, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < pCatalog->nEntryCount; ++i)
    {
        SendMessageW(hWndList, LB_ADDSTRING, 0, (LPARAM)pCatalog->entries[i].szName);
    }
    if (pCatalog->nEntryCount > 0)
    {
        SendMessageW(hWndList, LB_SETCURSEL, 0, 0);
    }
}

static BOOL WindowLayoutManager_PromptForName(HWND hWndParent, PCWSTR pszTitle, PCWSTR pszPrompt, PCWSTR pszInitialName, WCHAR* pszName, size_t cchName)
{
    WindowLayoutNameDialogContext context = { 0 };
    if (!pszName || cchName == 0)
    {
        return FALSE;
    }

    StringCchCopyW(context.szTitle, ARRAYSIZE(context.szTitle), pszTitle ? pszTitle : L"Save Window Layout");
    StringCchCopyW(context.szPrompt, ARRAYSIZE(context.szPrompt), pszPrompt ? pszPrompt : L"Layout name:");
    StringCchCopyW(context.szName, ARRAYSIZE(context.szName), pszInitialName ? pszInitialName : L"");
    INT_PTR result = DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_WINDOW_LAYOUT_NAME), hWndParent, WindowLayoutManager_NameDialogProc, (LPARAM)&context);
    if (result != IDOK)
    {
        return FALSE;
    }

    StringCchCopyW(pszName, cchName, context.szName);
    return TRUE;
}

static INT_PTR CALLBACK WindowLayoutManager_NameDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowLayoutNameDialogContext* pContext = (WindowLayoutNameDialogContext*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
        pContext = (WindowLayoutNameDialogContext*)lParam;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pContext);
        if (pContext)
        {
            SetWindowTextW(hWnd, pContext->szTitle);
            SetDlgItemTextW(hWnd, IDC_WINDOW_LAYOUT_NAME_PROMPT, pContext->szPrompt);
            SetDlgItemTextW(hWnd, IDC_WINDOW_LAYOUT_NAME_EDIT, pContext->szName);
            SendDlgItemMessageW(hWnd, IDC_WINDOW_LAYOUT_NAME_EDIT, EM_SETSEL, 0, -1);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (!pContext)
            {
                EndDialog(hWnd, IDCANCEL);
                return TRUE;
            }
            GetDlgItemTextW(hWnd, IDC_WINDOW_LAYOUT_NAME_EDIT, pContext->szName, ARRAYSIZE(pContext->szName));
            if (pContext->szName[0] == L'\0')
            {
                MessageBoxW(hWnd, L"Layout name cannot be empty.", L"Window Layout", MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            EndDialog(hWnd, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK WindowLayoutManager_ManageDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowLayoutManageDialogContext* pContext = (WindowLayoutManageDialogContext*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
        pContext = (WindowLayoutManageDialogContext*)lParam;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pContext);
        if (pContext)
        {
            WindowLayoutManager_FillListBox(GetDlgItem(hWnd, IDC_WINDOW_LAYOUT_LIST), &pContext->catalog);
        }
        return TRUE;

    case WM_COMMAND:
        if (!pContext)
        {
            break;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }

        if (LOWORD(wParam) == IDC_WINDOW_LAYOUT_UP ||
            LOWORD(wParam) == IDC_WINDOW_LAYOUT_DOWN ||
            LOWORD(wParam) == IDC_WINDOW_LAYOUT_RENAME ||
            LOWORD(wParam) == IDC_WINDOW_LAYOUT_DELETE)
        {
            HWND hWndList = GetDlgItem(hWnd, IDC_WINDOW_LAYOUT_LIST);
            int nIndex = (int)SendMessageW(hWndList, LB_GETCURSEL, 0, 0);
            if (nIndex < 0 || nIndex >= pContext->catalog.nEntryCount)
            {
                return TRUE;
            }

            switch (LOWORD(wParam))
            {
            case IDC_WINDOW_LAYOUT_UP:
                if (nIndex > 0 && WindowLayoutCatalog_Move(&pContext->catalog, nIndex, nIndex - 1))
                {
                    WindowLayoutManager_SaveCatalog(&pContext->catalog);
                    WindowLayoutManager_FillListBox(hWndList, &pContext->catalog);
                    SendMessageW(hWndList, LB_SETCURSEL, nIndex - 1, 0);
                    WindowLayoutManager_RefreshApplyMenu(pContext->pPanitentWindow);
                }
                return TRUE;

            case IDC_WINDOW_LAYOUT_DOWN:
                if (nIndex + 1 < pContext->catalog.nEntryCount && WindowLayoutCatalog_Move(&pContext->catalog, nIndex, nIndex + 1))
                {
                    WindowLayoutManager_SaveCatalog(&pContext->catalog);
                    WindowLayoutManager_FillListBox(hWndList, &pContext->catalog);
                    SendMessageW(hWndList, LB_SETCURSEL, nIndex + 1, 0);
                    WindowLayoutManager_RefreshApplyMenu(pContext->pPanitentWindow);
                }
                return TRUE;

            case IDC_WINDOW_LAYOUT_RENAME:
            {
                WCHAR szName[WINDOW_LAYOUT_NAME_MAX] = L"";
                StringCchCopyW(szName, ARRAYSIZE(szName), pContext->catalog.entries[nIndex].szName);
                if (!WindowLayoutManager_PromptForName(hWnd, L"Rename Window Layout", L"Layout name:", szName, szName, ARRAYSIZE(szName)))
                {
                    return TRUE;
                }
                if (!WindowLayoutCatalog_Rename(&pContext->catalog, nIndex, szName))
                {
                    MessageBoxW(hWnd, L"A layout with this name already exists.", L"Window Layout", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }
                WindowLayoutManager_SaveCatalog(&pContext->catalog);
                WindowLayoutManager_FillListBox(hWndList, &pContext->catalog);
                SendMessageW(hWndList, LB_SETCURSEL, nIndex, 0);
                WindowLayoutManager_RefreshApplyMenu(pContext->pPanitentWindow);
                return TRUE;
            }

            case IDC_WINDOW_LAYOUT_DELETE:
            {
                uint32_t uId = pContext->catalog.entries[nIndex].uId;
                PTSTR pszDockLayoutPath = NULL;
                PTSTR pszDockFloatingPath = NULL;
                PTSTR pszFloatDocLayoutPath = NULL;
                WindowLayoutManager_GetDockLayoutPath(uId, &pszDockLayoutPath);
                WindowLayoutManager_GetDockFloatingPath(uId, &pszDockFloatingPath);
                WindowLayoutManager_GetFloatDocLayoutPath(uId, &pszFloatDocLayoutPath);
                if (pszDockLayoutPath)
                {
                    DeleteFileW(pszDockLayoutPath);
                    free(pszDockLayoutPath);
                }
                if (pszDockFloatingPath)
                {
                    DeleteFileW(pszDockFloatingPath);
                    free(pszDockFloatingPath);
                }
                if (pszFloatDocLayoutPath)
                {
                    DeleteFileW(pszFloatDocLayoutPath);
                    free(pszFloatDocLayoutPath);
                }
                WindowLayoutCatalog_Delete(&pContext->catalog, nIndex);
                WindowLayoutManager_SaveCatalog(&pContext->catalog);
                WindowLayoutManager_FillListBox(hWndList, &pContext->catalog);
                if (pContext->catalog.nEntryCount > 0)
                {
                    if (nIndex >= pContext->catalog.nEntryCount)
                    {
                        nIndex = pContext->catalog.nEntryCount - 1;
                    }
                    SendMessageW(hWndList, LB_SETCURSEL, nIndex, 0);
                }
                WindowLayoutManager_RefreshApplyMenu(pContext->pPanitentWindow);
                return TRUE;
            }
            }
        }
        break;
    }

    return FALSE;
}

BOOL WindowLayoutManager_HandleCommand(PanitentWindow* pPanitentWindow, UINT cmdId)
{
    HWND hWndParent = pPanitentWindow ? Window_GetHWND((Window*)pPanitentWindow) : NULL;

    if (!pPanitentWindow)
    {
        return FALSE;
    }

    if (cmdId >= IDM_WINDOW_APPLY_LAYOUT_BASE && cmdId <= IDM_WINDOW_APPLY_LAYOUT_LAST)
    {
        WindowLayoutCatalog catalog = { 0 };
        WindowLayoutManager_LoadCatalog(&catalog);
        int nIndex = (int)(cmdId - IDM_WINDOW_APPLY_LAYOUT_BASE);
        if (nIndex >= 0 && nIndex < catalog.nEntryCount)
        {
            if (!WindowLayoutManager_ApplyProfile(pPanitentWindow, catalog.entries[nIndex].uId))
            {
                MessageBoxW(hWndParent, L"Failed to apply the selected window layout.", L"Window Layout", MB_OK | MB_ICONERROR);
            }
        }
        return TRUE;
    }

    switch (cmdId)
    {
    case IDM_WINDOW_APPLY_LAYOUT_NONE:
        return TRUE;

    case IDM_WINDOW_SAVE_LAYOUT:
    {
        WindowLayoutCatalog catalog = { 0 };
        WCHAR szName[WINDOW_LAYOUT_NAME_MAX] = L"";
        uint32_t uId;

        WindowLayoutManager_LoadCatalog(&catalog);
        if (!WindowLayoutManager_PromptForName(hWndParent, L"Save Window Layout", L"Layout name:", L"", szName, ARRAYSIZE(szName)))
        {
            return TRUE;
        }

        int nExisting = WindowLayoutCatalog_FindByName(&catalog, szName);
        if (nExisting >= 0)
        {
            uId = catalog.entries[nExisting].uId;
        }
        else {
            uId = WindowLayoutCatalog_AllocateId(&catalog);
            if (!WindowLayoutCatalog_Add(&catalog, uId, szName))
            {
                MessageBoxW(hWndParent, L"Failed to create the window layout entry.", L"Window Layout", MB_OK | MB_ICONERROR);
                return TRUE;
            }
        }

        if (!WindowLayoutManager_SaveProfile(pPanitentWindow, uId) || !WindowLayoutManager_SaveCatalog(&catalog))
        {
            MessageBoxW(hWndParent, L"Failed to save the current window layout.", L"Window Layout", MB_OK | MB_ICONERROR);
        }
        WindowLayoutManager_RefreshApplyMenu(pPanitentWindow);
        return TRUE;
    }

    case IDM_WINDOW_MANAGE_LAYOUTS:
    {
        WindowLayoutManageDialogContext context = { 0 };
        context.pPanitentWindow = pPanitentWindow;
        WindowLayoutManager_LoadCatalog(&context.catalog);
        DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_WINDOW_LAYOUT_MANAGE), hWndParent, WindowLayoutManager_ManageDialogProc, (LPARAM)&context);
        WindowLayoutManager_RefreshApplyMenu(pPanitentWindow);
        return TRUE;
    }

    case IDM_WINDOW_RESET_LAYOUT:
        if (!WindowLayoutManager_ApplyDefault(pPanitentWindow))
        {
            MessageBoxW(hWndParent, L"Failed to reset the window layout.", L"Window Layout", MB_OK | MB_ICONERROR);
        }
        return TRUE;
    }

    return FALSE;
}

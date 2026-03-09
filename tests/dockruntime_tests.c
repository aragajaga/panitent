#include <assert.h>
#include <ole2.h>

#include "../src/dockhost.h"
#include "../src/dockhostdrag.h"
#include "../src/dockhostmodelapply.h"
#include "../src/docklayout.h"
#include "../src/documentdocktransition.h"
#include "../src/dockfloatingmodel.h"
#include "../src/dockfloatingpersist.h"
#include "../src/floatingchildhost.h"
#include "../src/floatingdocumenthost.h"
#include "../src/floatingdocumentlayoutmodel.h"
#include "../src/floatingdocumentlayoutpersist.h"
#include "../src/floatingdocumentsessionpersist.h"
#include "../src/floatingwindowcontainer.h"
#include "../src/resource.h"
#include "../src/windowlayoutcatalog.h"
#include "../src/canvas.h"
#include "../src/document.h"
#include "../src/dockmodel.h"
#include "../src/dockmodelops.h"
#include "../src/dockshell.h"
#include "../src/panitentapp.h"
#include "../src/panitentwindow.h"
#include "../src/workspacecontainer.h"
#include "../src/windowlayoutmanager.h"
#include "../src/windowlayoutprofile.h"
#include "../src/shell/pathutil.h"
#include "../src/win32/window.h"
#include "../src/win32/windowmap.h"

typedef struct DockRuntimeFixture
{
    PanitentApp* pApp;
    Window* pFrame;
    HWND hWndFrame;
    DockHostWindow* pDockHostWindow;
    HWND hWndDockHost;
    PanitentWindow panitentWindow;
} DockRuntimeFixture;

typedef struct FloatingCountContext
{
    int nToolPanels;
    int nToolHosts;
    int nDocumentWorkspaces;
    int nDocumentHosts;
} FloatingCountContext;

static int g_runtimeWindowLayoutMessageCount = 0;
static WCHAR g_runtimeWindowLayoutPromptName[WINDOW_LAYOUT_NAME_MAX] = L"";
static int g_runtimeFloatingDocumentLayoutRestoreFailCountdown = -1;
static int g_runtimeDockFloatingRestoreFailCountdown = -1;

static BOOL runtime_is_class_name(HWND hWnd, PCWSTR pszClassName);

static BOOL runtime_fail_floating_document_create(
    DockHostWindow* pDockHostTarget,
    HWND hWndChild,
    const RECT* pWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut)
{
    UNREFERENCED_PARAMETER(pDockHostTarget);
    UNREFERENCED_PARAMETER(hWndChild);
    UNREFERENCED_PARAMETER(pWindowRect);
    UNREFERENCED_PARAMETER(bStartMove);
    UNREFERENCED_PARAMETER(ptMoveScreen);

    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);
    return FALSE;
}

static BOOL runtime_fail_floating_tool_restore(const DockFloatingLayoutEntry* pEntry)
{
    UNREFERENCED_PARAMETER(pEntry);
    return FALSE;
}

static BOOL runtime_fail_floating_tool_restore_on_countdown(const DockFloatingLayoutEntry* pEntry)
{
    UNREFERENCED_PARAMETER(pEntry);

    if (g_runtimeDockFloatingRestoreFailCountdown > 0)
    {
        g_runtimeDockFloatingRestoreFailCountdown--;
        return TRUE;
    }

    PanitentDockFloating_SetRestoreEntryTestHook(NULL);
    return FALSE;
}

static BOOL runtime_fail_floating_document_layout_restore_on_countdown(const FloatingDocumentLayoutEntry* pEntry)
{
    UNREFERENCED_PARAMETER(pEntry);

    if (g_runtimeFloatingDocumentLayoutRestoreFailCountdown > 0)
    {
        g_runtimeFloatingDocumentLayoutRestoreFailCountdown--;
        return TRUE;
    }

    PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(NULL);
    return FALSE;
}

static int runtime_capture_window_layout_message(HWND hWndParent, PCWSTR pszText, PCWSTR pszCaption, UINT uType)
{
    UNREFERENCED_PARAMETER(hWndParent);
    UNREFERENCED_PARAMETER(pszText);
    UNREFERENCED_PARAMETER(pszCaption);
    UNREFERENCED_PARAMETER(uType);
    g_runtimeWindowLayoutMessageCount++;
    return IDOK;
}

static BOOL runtime_fail_window_layout_save_catalog(const void* pCatalog)
{
    UNREFERENCED_PARAMETER(pCatalog);
    return FALSE;
}

static BOOL runtime_fail_window_layout_save_profile(PanitentWindow* pPanitentWindow, uint32_t uId)
{
    UNREFERENCED_PARAMETER(pPanitentWindow);
    UNREFERENCED_PARAMETER(uId);
    return FALSE;
}

static BOOL runtime_capture_window_layout_prompt(
    HWND hWndParent,
    PCWSTR pszTitle,
    PCWSTR pszPrompt,
    PCWSTR pszInitialName,
    WCHAR* pszName,
    size_t cchName)
{
    UNREFERENCED_PARAMETER(hWndParent);
    UNREFERENCED_PARAMETER(pszTitle);
    UNREFERENCED_PARAMETER(pszPrompt);
    UNREFERENCED_PARAMETER(pszInitialName);

    if (!pszName || cchName == 0 || g_runtimeWindowLayoutPromptName[0] == L'\0')
    {
        return FALSE;
    }

    StringCchCopyW(pszName, cchName, g_runtimeWindowLayoutPromptName);
    return TRUE;
}

static BOOL runtime_fail_document_dock_once(
    DockHostWindow* pTargetDockHostWindow,
    HWND hWndChild,
    const DockTargetHit* pDockTarget,
    int iDockSize)
{
    UNREFERENCED_PARAMETER(pTargetDockHostWindow);
    UNREFERENCED_PARAMETER(hWndChild);
    UNREFERENCED_PARAMETER(pDockTarget);
    UNREFERENCED_PARAMETER(iDockSize);
    DocumentDockTransition_SetDockTestHook(NULL);
    return FALSE;
}

static HWND runtime_find_floating_document_window(void)
{
    for (HWND hWnd = GetTopWindow(NULL); hWnd; hWnd = GetNextWindow(hWnd, GW_HWNDNEXT))
    {
        if (!runtime_is_class_name(hWnd, L"__FloatingWindowContainer"))
        {
            continue;
        }

        FloatingWindowContainer* pFloating = (FloatingWindowContainer*)WindowMap_Get(hWnd);
        if (!pFloating || pFloating->nDockPolicy != FLOAT_DOCK_POLICY_DOCUMENT)
        {
            continue;
        }

        if (pFloating->hWndChild && IsWindow(pFloating->hWndChild))
        {
            return hWnd;
        }
    }

    return NULL;
}

static HWND runtime_find_floating_tool_host_window(void)
{
    for (HWND hWnd = GetTopWindow(NULL); hWnd; hWnd = GetNextWindow(hWnd, GW_HWNDNEXT))
    {
        if (!runtime_is_class_name(hWnd, L"__FloatingWindowContainer"))
        {
            continue;
        }

        FloatingWindowContainer* pFloating = (FloatingWindowContainer*)WindowMap_Get(hWnd);
        if (!pFloating || pFloating->nDockPolicy != FLOAT_DOCK_POLICY_PANEL)
        {
            continue;
        }

        if (pFloating->hWndChild && runtime_is_class_name(pFloating->hWndChild, L"__DockHostWindow"))
        {
            return hWnd;
        }
    }

    return NULL;
}

static BOOL CALLBACK runtime_destroy_floating_windows_proc(HWND hWnd, LPARAM lParam)
{
    DWORD processId = 0;
    UNREFERENCED_PARAMETER(lParam);

    if (!hWnd || !IsWindow(hWnd))
    {
        return TRUE;
    }

    GetWindowThreadProcessId(hWnd, &processId);
    if (processId != GetCurrentProcessId())
    {
        return TRUE;
    }

    if (runtime_is_class_name(hWnd, L"__FloatingWindowContainer"))
    {
        DestroyWindow(hWnd);
    }

    return TRUE;
}

static void runtime_destroy_all_floating_windows(void)
{
    EnumWindows(runtime_destroy_floating_windows_proc, 0);
}

static TreeNode* runtime_find_live_node_by_name(TreeNode* pNode, PCWSTR pszName)
{
    if (!pNode || !pszName)
    {
        return NULL;
    }

    DockData* pDockData = (DockData*)pNode->data;
    if (pDockData && wcscmp(pDockData->lpszName, pszName) == 0)
    {
        return pNode;
    }

    TreeNode* pFound = runtime_find_live_node_by_name(pNode->node1, pszName);
    if (pFound)
    {
        return pFound;
    }

    return runtime_find_live_node_by_name(pNode->node2, pszName);
}

static TreeNode* runtime_find_live_node_by_hwnd(TreeNode* pNode, HWND hWnd)
{
    if (!pNode || !hWnd)
    {
        return NULL;
    }

    DockData* pDockData = (DockData*)pNode->data;
    if (pDockData && pDockData->hWnd == hWnd)
    {
        return pNode;
    }

    TreeNode* pFound = runtime_find_live_node_by_hwnd(pNode->node1, hWnd);
    if (pFound)
    {
        return pFound;
    }

    return runtime_find_live_node_by_hwnd(pNode->node2, hWnd);
}

static DockModelNode* runtime_find_model_node_by_name(DockModelNode* pNode, PCWSTR pszName)
{
    if (!pNode || !pszName)
    {
        return NULL;
    }

    if (wcscmp(pNode->szName, pszName) == 0)
    {
        return pNode;
    }

    DockModelNode* pFound = runtime_find_model_node_by_name(pNode->pChild1, pszName);
    if (pFound)
    {
        return pFound;
    }

    return runtime_find_model_node_by_name(pNode->pChild2, pszName);
}

static DockModelNode* runtime_find_model_zone(DockModelNode* pNode, int nDockSide)
{
    if (!pNode)
    {
        return NULL;
    }

    if (pNode->nRole == DOCK_ROLE_ZONE && pNode->nDockSide == nDockSide)
    {
        return pNode;
    }

    DockModelNode* pFound = runtime_find_model_zone(pNode->pChild1, nDockSide);
    if (pFound)
    {
        return pFound;
    }

    return runtime_find_model_zone(pNode->pChild2, nDockSide);
}

static int runtime_count_live_role(TreeNode* pNode, DockNodeRole nRole)
{
    if (!pNode || !pNode->data)
    {
        return 0;
    }

    DockData* pDockData = (DockData*)pNode->data;
    int nCount = pDockData->nRole == nRole ? 1 : 0;
    return nCount +
        runtime_count_live_role(pNode->node1, nRole) +
        runtime_count_live_role(pNode->node2, nRole);
}

static int runtime_count_model_role(const DockModelNode* pNode, DockNodeRole nRole)
{
    if (!pNode)
    {
        return 0;
    }

    int nCount = pNode->nRole == nRole ? 1 : 0;
    return nCount +
        runtime_count_model_role(pNode->pChild1, nRole) +
        runtime_count_model_role(pNode->pChild2, nRole);
}

static BOOL runtime_model_subtree_contains_name(const DockModelNode* pNode, PCWSTR pszName)
{
    if (!pNode || !pszName)
    {
        return FALSE;
    }

    if (wcscmp(pNode->szName, pszName) == 0)
    {
        return TRUE;
    }

    return runtime_model_subtree_contains_name(pNode->pChild1, pszName) ||
        runtime_model_subtree_contains_name(pNode->pChild2, pszName);
}

static void runtime_assert_model_equal(const DockModelNode* pActual, const DockModelNode* pExpected)
{
    assert((pActual == NULL) == (pExpected == NULL));
    if (!pActual || !pExpected)
    {
        return;
    }

    assert(pActual->nRole == pExpected->nRole);
    if (pExpected->uNodeId != 0)
    {
        assert(pActual->uNodeId == pExpected->uNodeId);
    }
    assert(pActual->uViewId == pExpected->uViewId);
    assert(pActual->nPaneKind == pExpected->nPaneKind);
    assert(pActual->nDockSide == pExpected->nDockSide);
    assert(pActual->dwStyle == pExpected->dwStyle);
    assert(pActual->fGripPos == pExpected->fGripPos);
    assert(pActual->iGripPos == pExpected->iGripPos);
    assert(pActual->bShowCaption == pExpected->bShowCaption);
    assert(pActual->bCollapsed == pExpected->bCollapsed);
    assert(wcscmp(pActual->szName, pExpected->szName) == 0);
    assert(wcscmp(pActual->szCaption, pExpected->szCaption) == 0);
    assert(wcscmp(pActual->szActiveTabName, pExpected->szActiveTabName) == 0);
    runtime_assert_model_equal(pActual->pChild1, pExpected->pChild1);
    runtime_assert_model_equal(pActual->pChild2, pExpected->pChild2);
}

static void runtime_assert_model_semantically_equal(const DockModelNode* pActual, const DockModelNode* pExpected)
{
    assert((pActual == NULL) == (pExpected == NULL));
    if (!pActual || !pExpected)
    {
        return;
    }

    assert(pActual->nRole == pExpected->nRole);
    if (pExpected->uNodeId != 0)
    {
        assert(pActual->uNodeId == pExpected->uNodeId);
    }
    assert(pActual->uViewId == pExpected->uViewId);
    assert(pActual->nPaneKind == pExpected->nPaneKind);
    assert(pActual->nDockSide == pExpected->nDockSide);
    assert(pActual->dwStyle == pExpected->dwStyle);
    assert(pActual->fGripPos == pExpected->fGripPos);
    assert(pActual->bShowCaption == pExpected->bShowCaption);
    assert(pActual->bCollapsed == pExpected->bCollapsed);
    assert(wcscmp(pActual->szName, pExpected->szName) == 0);
    assert(wcscmp(pActual->szCaption, pExpected->szCaption) == 0);
    assert(wcscmp(pActual->szActiveTabName, pExpected->szActiveTabName) == 0);
    runtime_assert_model_semantically_equal(pActual->pChild1, pExpected->pChild1);
    runtime_assert_model_semantically_equal(pActual->pChild2, pExpected->pChild2);
}

static BOOL runtime_is_class_name(HWND hWnd, PCWSTR pszClassName)
{
    WCHAR szClassName[64] = L"";
    if (!hWnd || !IsWindow(hWnd) || !pszClassName)
    {
        return FALSE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return wcscmp(szClassName, pszClassName) == 0;
}

static void runtime_reset_app_state(PanitentApp* pApp)
{
    if (!pApp)
    {
        return;
    }

    pApp->m_pWorkspaceContainer = NULL;
    pApp->m_pPaletteWindow = NULL;
    PanitentApp_SetOptionBar(pApp, NULL);
}

static BOOL runtime_fixture_init(DockRuntimeFixture* pFixture)
{
    if (!pFixture)
    {
        return FALSE;
    }

    memset(pFixture, 0, sizeof(*pFixture));
    pFixture->pApp = PanitentApp_Instance();
    if (!pFixture->pApp)
    {
        return FALSE;
    }

    runtime_reset_app_state(pFixture->pApp);
    runtime_destroy_all_floating_windows();

    pFixture->pFrame = Window_Create();
    if (!pFixture->pFrame)
    {
        return FALSE;
    }

    pFixture->hWndFrame = Window_CreateWindow(pFixture->pFrame, NULL);
    if (!pFixture->hWndFrame || !IsWindow(pFixture->hWndFrame))
    {
        return FALSE;
    }

    SetWindowPos(pFixture->hWndFrame, NULL, 100, 100, 1280, 900, SWP_NOZORDER | SWP_NOACTIVATE);

    pFixture->pDockHostWindow = DockHostWindow_Create(pFixture->pApp);
    if (!pFixture->pDockHostWindow)
    {
        return FALSE;
    }

    pFixture->hWndDockHost = Window_CreateWindow((Window*)pFixture->pDockHostWindow, pFixture->hWndFrame);
    if (!pFixture->hWndDockHost || !IsWindow(pFixture->hWndDockHost))
    {
        return FALSE;
    }

    SetWindowPos(pFixture->hWndDockHost, NULL, 0, 0, 1280, 900, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(pFixture->hWndFrame, SW_HIDE);

    TreeNode* pRoot = DockShell_CreateRootNode();
    if (!pRoot || !pRoot->data)
    {
        return FALSE;
    }

    RECT rcDockHost = { 0 };
    GetClientRect(pFixture->hWndDockHost, &rcDockHost);
    ((DockData*)pRoot->data)->rc = rcDockHost;
    DockHostWindow_SetRoot(pFixture->pDockHostWindow, pRoot);
    PanitentApp_DockHostInit(pFixture->pApp, pFixture->pDockHostWindow, pRoot);
    DockHostWindow_Rearrange(pFixture->pDockHostWindow);

    memset(&pFixture->panitentWindow, 0, sizeof(pFixture->panitentWindow));
    pFixture->panitentWindow.m_pDockHostWindow = pFixture->pDockHostWindow;

    return runtime_find_live_node_by_name(DockHostWindow_GetRoot(pFixture->pDockHostWindow), L"WorkspaceContainer") != NULL;
}

static void runtime_fixture_destroy(DockRuntimeFixture* pFixture)
{
    if (!pFixture)
    {
        return;
    }

    if (pFixture->hWndFrame && IsWindow(pFixture->hWndFrame))
    {
        DestroyWindow(pFixture->hWndFrame);
    }

    runtime_destroy_all_floating_windows();
    runtime_reset_app_state(pFixture->pApp);
    memset(pFixture, 0, sizeof(*pFixture));
}

static HWND runtime_get_live_hwnd_by_name(DockHostWindow* pDockHostWindow, PCWSTR pszName)
{
    TreeNode* pNode = runtime_find_live_node_by_name(
        pDockHostWindow ? DockHostWindow_GetRoot(pDockHostWindow) : NULL,
        pszName);
    DockData* pDockData = pNode ? (DockData*)pNode->data : NULL;
    return pDockData ? pDockData->hWnd : NULL;
}

static void runtime_delete_profile_bundle(uint32_t uId)
{
    PTSTR pszPath = NULL;

    if (WindowLayoutProfile_GetDockLayoutPath(uId, &pszPath))
    {
        DeleteFileW(pszPath);
        free(pszPath);
    }

    pszPath = NULL;
    if (WindowLayoutProfile_GetDockFloatingPath(uId, &pszPath))
    {
        DeleteFileW(pszPath);
        free(pszPath);
    }

    pszPath = NULL;
    if (WindowLayoutProfile_GetFloatDocLayoutPath(uId, &pszPath))
    {
        DeleteFileW(pszPath);
        free(pszPath);
    }
}

static void runtime_delete_window_layout_catalog_file(void)
{
    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    if (!pszCatalogPath)
    {
        return;
    }

    DeleteFileW(pszCatalogPath);
    free(pszCatalogPath);
}

static void runtime_delete_floating_document_session_file(void)
{
    PTSTR pszFilePath = NULL;
    GetFloatingDocumentSessionFilePath(&pszFilePath);
    if (!pszFilePath)
    {
        return;
    }

    DeleteFileW(pszFilePath);
    free(pszFilePath);
}

static BOOL CALLBACK runtime_enum_floating_windows(HWND hWnd, LPARAM lParam)
{
    FloatingCountContext* pContext = (FloatingCountContext*)lParam;
    DWORD processId = 0;

    if (!pContext || !hWnd || !IsWindow(hWnd))
    {
        return TRUE;
    }

    GetWindowThreadProcessId(hWnd, &processId);
    if (processId != GetCurrentProcessId() || !runtime_is_class_name(hWnd, L"__FloatingWindowContainer"))
    {
        return TRUE;
    }

    FloatingWindowContainer* pFloating = (FloatingWindowContainer*)WindowMap_Get(hWnd);
    if (!pFloating)
    {
        return TRUE;
    }

    if (pFloating->nDockPolicy == FLOAT_DOCK_POLICY_PANEL)
    {
        if (runtime_is_class_name(pFloating->hWndChild, L"__DockHostWindow"))
        {
            pContext->nToolHosts++;
        }
        else {
            pContext->nToolPanels++;
        }
    }
    else if (pFloating->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        if (runtime_is_class_name(pFloating->hWndChild, L"__DockHostWindow"))
        {
            pContext->nDocumentHosts++;
        }
        else {
            pContext->nDocumentWorkspaces++;
        }
    }

    return TRUE;
}

static void runtime_collect_floating_counts(FloatingCountContext* pContext)
{
    if (!pContext)
    {
        return;
    }

    memset(pContext, 0, sizeof(*pContext));
    EnumWindows(runtime_enum_floating_windows, (LPARAM)pContext);
}

static int test_runtime_tool_model_docking_moves_panel_between_zones(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndToolbox = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"Toolbox");
    HWND hWndGLWindow = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"GLWindow");
    assert(hWndToolbox && IsWindow(hWndToolbox));
    assert(hWndGLWindow && IsWindow(hWndGLWindow));

    assert(DockHostModelApply_RemoveToolWindow(fixture.pDockHostWindow, hWndGLWindow, TRUE));

    DockTargetHit targetHit = { 0 };
    targetHit.nDockSide = DKS_BOTTOM;
    targetHit.bLocalTarget = TRUE;
    targetHit.hWndAnchor = hWndToolbox;
    assert(DockHostWindow_DockHWNDToTarget(fixture.pDockHostWindow, hWndGLWindow, &targetHit, 180));

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pApplied != NULL);

    DockModelNode* pLeftZone = runtime_find_model_zone(pApplied, DKS_LEFT);
    DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
    assert(pLeftZone != NULL);
    assert(pRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pLeftZone, L"Toolbox"));
    assert(runtime_model_subtree_contains_name(pLeftZone, L"GLWindow"));
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    DockModel_Destroy(pApplied);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_window_layout_apply_invalid_bundle_rolls_back(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    DockModelNode* pBefore = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pInvalid = DockModelOps_CloneTree(pBefore);
    assert(pBefore != NULL);
    assert(pInvalid != NULL);

    DockModelNode* pWorkspace = runtime_find_model_node_by_name(pInvalid, L"WorkspaceContainer");
    assert(pWorkspace != NULL);
    assert(DockModelOps_RemoveNodeById(&pInvalid, pWorkspace->uNodeId));

    assert(!WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pInvalid, NULL, NULL));

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pAfter != NULL);
    runtime_assert_model_semantically_equal(pAfter, pBefore);

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pInvalid);
    DockModel_Destroy(pBefore);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_window_layout_apply_and_reset_preserves_workspace(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pBase = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pTarget = DockModelOps_CloneTree(pBase);
    assert(pBase != NULL);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    DockModelNode* pToolbox = runtime_find_model_node_by_name(pTarget, L"Toolbox");
    assert(pGLWindow != NULL);
    assert(pToolbox != NULL);

    DockModelNode* pIncoming = DockModelOps_CloneTree(pGLWindow);
    assert(pIncoming != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));
    assert(DockModelOps_DockPanelAroundNode(pTarget, pToolbox->uNodeId, DKS_BOTTOM, pIncoming));

    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pTarget, NULL, NULL));

    HWND hWndWorkspaceAfterApply = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterApply == hWndWorkspaceBefore);

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pLeftZone = runtime_find_model_zone(pApplied, DKS_LEFT);
    DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
    assert(pApplied != NULL);
    assert(pLeftZone != NULL);
    assert(pRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pLeftZone, L"GLWindow"));
    assert(runtime_model_subtree_contains_name(pLeftZone, L"Toolbox"));
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));

    HWND hWndWorkspaceAfterReset = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterReset == hWndWorkspaceBefore);

    DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pResetLeftZone = runtime_find_model_zone(pReset, DKS_LEFT);
    DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
    DockModelNode* pResetBottomZone = runtime_find_model_zone(pReset, DKS_BOTTOM);
    assert(pReset != NULL);
    assert(runtime_find_model_node_by_name(pReset, L"WorkspaceContainer") != NULL);
    assert(pResetLeftZone != NULL);
    assert(pResetRightZone != NULL);
    assert(pResetBottomZone != NULL);
    assert(runtime_model_subtree_contains_name(pResetLeftZone, L"Toolbox"));
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"Palette"));
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"Layers"));
    assert(runtime_model_subtree_contains_name(pResetBottomZone, L"Option Bar"));

    DockModel_Destroy(pReset);
    DockModel_Destroy(pApplied);
    DockModel_Destroy(pTarget);
    DockModel_Destroy(pBase);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_named_layout_profile_switch_round_trip(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uIdA = 0x7A000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    const uint32_t uIdB = uIdA + 1u;
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pLayoutA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingA = { 0 };
    FloatingDocumentLayoutModel floatDocsA = { 0 };
    assert(pLayoutA != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingA));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsA));
    assert(WindowLayoutProfile_SaveBundle(uIdA, pLayoutA, &floatingA, &floatDocsA));

    DockModelNode* pTarget = DockModelOps_CloneTree(pLayoutA);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    DockModelNode* pToolbox = runtime_find_model_node_by_name(pTarget, L"Toolbox");
    assert(pTarget != NULL);
    assert(pGLWindow != NULL);
    assert(pToolbox != NULL);

    DockModelNode* pIncoming = DockModelOps_CloneTree(pGLWindow);
    assert(pIncoming != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));
    assert(DockModelOps_DockPanelAroundNode(pTarget, pToolbox->uNodeId, DKS_BOTTOM, pIncoming));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pTarget, NULL, NULL));

    DockModelNode* pLayoutB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingB = { 0 };
    FloatingDocumentLayoutModel floatDocsB = { 0 };
    assert(pLayoutB != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingB));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsB));
    assert(WindowLayoutProfile_SaveBundle(uIdB, pLayoutB, &floatingB, &floatDocsB));

    DockModelNode* pLoadedLayout = NULL;
    DockFloatingLayoutFileModel loadedFloating = { 0 };
    FloatingDocumentLayoutModel loadedFloatDocs = { 0 };
    assert(WindowLayoutProfile_LoadBundle(uIdA, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    HWND hWndWorkspaceAfterA = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterA == hWndWorkspaceBefore);

    DockModelNode* pAppliedA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZoneA = runtime_find_model_zone(pAppliedA, DKS_RIGHT);
    assert(pAppliedA != NULL);
    assert(pRightZoneA != NULL);
    assert(runtime_model_subtree_contains_name(pRightZoneA, L"GLWindow"));
    assert(!runtime_model_subtree_contains_name(runtime_find_model_zone(pAppliedA, DKS_LEFT), L"GLWindow"));

    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);

    pLoadedLayout = NULL;
    memset(&loadedFloating, 0, sizeof(loadedFloating));
    memset(&loadedFloatDocs, 0, sizeof(loadedFloatDocs));
    assert(WindowLayoutProfile_LoadBundle(uIdB, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    HWND hWndWorkspaceAfterB = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterB == hWndWorkspaceBefore);

    DockModelNode* pAppliedB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pLeftZoneB = runtime_find_model_zone(pAppliedB, DKS_LEFT);
    DockModelNode* pRightZoneB = runtime_find_model_zone(pAppliedB, DKS_RIGHT);
    assert(pAppliedB != NULL);
    assert(pLeftZoneB != NULL);
    assert(pRightZoneB != NULL);
    assert(runtime_model_subtree_contains_name(pLeftZoneB, L"GLWindow"));
    assert(!runtime_model_subtree_contains_name(pRightZoneB, L"GLWindow"));

    DockModel_Destroy(pAppliedB);
    DockModel_Destroy(pAppliedA);
    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);
    DockModel_Destroy(pTarget);
    DockModel_Destroy(pLayoutB);
    DockFloatingLayout_Destroy(&floatingB);
    FloatingDocumentLayoutModel_Destroy(&floatDocsB);
    DockModel_Destroy(pLayoutA);
    DockFloatingLayout_Destroy(&floatingA);
    FloatingDocumentLayoutModel_Destroy(&floatDocsA);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_apply_mixed_floating_layout_bundle(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        &floatDocModel));

    HWND hWndWorkspaceAfter = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfter == hWndWorkspaceBefore);

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
    assert(pApplied != NULL);
    assert(pRightZone != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
    assert(pReset != NULL);
    assert(pResetRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));

    DockModel_Destroy(pReset);
    DockModel_Destroy(pApplied);
    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_reapply_mixed_floating_layout_bundle_is_idempotent(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        &floatDocModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    assert(WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        &floatDocModel));

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
    assert(pApplied != NULL);
    assert(pRightZone != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModel_Destroy(pApplied);
    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_direct_floating_tool_restore_is_idempotent(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pTarget, NULL, NULL));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    assert(PanitentDockFloating_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatingModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);

    assert(PanitentDockFloating_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatingModel));

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);

    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_direct_floating_tool_host_restore_is_idempotent(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    DockModelNode* pPalette = runtime_find_model_node_by_name(pTarget, L"Palette");
    assert(pGLWindow != NULL);
    assert(pPalette != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));
    assert(DockModelOps_RemoveNodeById(&pTarget, pPalette->uNodeId));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pTarget, NULL, NULL));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    DockModelNode floatRoot = { 0 };
    DockModelNode floatSplit = { 0 };
    DockModelNode floatGL = { 0 };
    DockModelNode floatPalette = { 0 };

    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 500, 520);
    floatingModel.entries[0].iDockSizeHint = 260;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_HOST;
    floatingModel.entries[0].pLayoutModel = &floatRoot;

    floatRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatRoot.szName, ARRAYSIZE(floatRoot.szName), L"Root");
    floatRoot.pChild1 = &floatSplit;

    floatSplit.nRole = DOCK_ROLE_PANEL_SPLIT;
    floatSplit.nPaneKind = DOCK_PANE_TOOL;
    floatSplit.dwStyle = DGP_RELATIVE | DGD_VERTICAL;
    floatSplit.fGripPos = 0.5f;
    wcscpy_s(floatSplit.szName, ARRAYSIZE(floatSplit.szName), L"DockShell.PanelSplit");
    floatSplit.pChild1 = &floatGL;
    floatSplit.pChild2 = &floatPalette;

    floatGL.nRole = DOCK_ROLE_PANEL;
    floatGL.nPaneKind = DOCK_PANE_TOOL;
    floatGL.uViewId = PNT_DOCK_VIEW_GLWINDOW;
    floatGL.bShowCaption = TRUE;
    wcscpy_s(floatGL.szName, ARRAYSIZE(floatGL.szName), L"GLWindow");
    wcscpy_s(floatGL.szCaption, ARRAYSIZE(floatGL.szCaption), L"GLWindow");

    floatPalette.nRole = DOCK_ROLE_PANEL;
    floatPalette.nPaneKind = DOCK_PANE_TOOL;
    floatPalette.uViewId = PNT_DOCK_VIEW_PALETTE;
    floatPalette.bShowCaption = TRUE;
    wcscpy_s(floatPalette.szName, ARRAYSIZE(floatPalette.szName), L"Palette");
    wcscpy_s(floatPalette.szCaption, ARRAYSIZE(floatPalette.szCaption), L"Palette");

    assert(PanitentDockFloating_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatingModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 1);

    assert(PanitentDockFloating_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatingModel));

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 1);

    HWND hWndFloating = runtime_find_floating_tool_host_window();
    assert(hWndFloating && IsWindow(hWndFloating));
    FloatingWindowContainer* pFloating = (FloatingWindowContainer*)WindowMap_Get(hWndFloating);
    assert(pFloating != NULL);
    assert(pFloating->hWndChild && runtime_is_class_name(pFloating->hWndChild, L"__DockHostWindow"));

    DockHostWindow* pFloatingDockHost = (DockHostWindow*)WindowMap_Get(pFloating->hWndChild);
    assert(pFloatingDockHost != NULL);
    DockModelNode* pApplied = DockModel_CaptureHostLayout(pFloatingDockHost);
    assert(pApplied != NULL);
    assert(runtime_model_subtree_contains_name(pApplied, L"GLWindow"));
    assert(runtime_model_subtree_contains_name(pApplied, L"Palette"));

    DockModel_Destroy(pApplied);
    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_direct_floating_tool_restore_strict_mode_rolls_back_on_partial_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    DockModelNode* pPalette = runtime_find_model_node_by_name(pTarget, L"Palette");
    assert(pGLWindow != NULL);
    assert(pPalette != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));
    assert(DockModelOps_RemoveNodeById(&pTarget, pPalette->uNodeId));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pTarget, NULL, NULL));

    DockFloatingLayoutFileModel activeModel = { 0 };
    activeModel.nEntries = 1;
    SetRect(&activeModel.entries[0].rcWindow, 140, 160, 440, 480);
    activeModel.entries[0].iDockSizeHint = 240;
    activeModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    activeModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;
    assert(PanitentDockFloating_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &activeModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);

    DockFloatingLayoutFileModel targetModel = { 0 };
    targetModel.nEntries = 2;
    SetRect(&targetModel.entries[0].rcWindow, 140, 160, 440, 480);
    targetModel.entries[0].iDockSizeHint = 240;
    targetModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    targetModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;
    SetRect(&targetModel.entries[1].rcWindow, 500, 160, 800, 480);
    targetModel.entries[1].iDockSizeHint = 240;
    targetModel.entries[1].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    targetModel.entries[1].nViewId = PNT_DOCK_VIEW_PALETTE;

    g_runtimeDockFloatingRestoreFailCountdown = 1;
    PanitentDockFloating_SetRestoreEntryTestHook(runtime_fail_floating_tool_restore_on_countdown);
    assert(!PanitentDockFloating_RestoreModelEx(fixture.pApp, fixture.pDockHostWindow, &targetModel, TRUE));
    PanitentDockFloating_SetRestoreEntryTestHook(NULL);
    g_runtimeDockFloatingRestoreFailCountdown = -1;

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
    assert(pApplied != NULL);
    assert(pRightZone != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));
    assert(!runtime_model_subtree_contains_name(pRightZone, L"Palette"));

    HWND hWndFloating = runtime_find_floating_tool_host_window();
    assert(hWndFloating == NULL);

    DockModel_Destroy(pApplied);
    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_reapply_mixed_layout_failure_rolls_back_to_existing_mixed_state(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        &floatDocModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    assert(!WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        &floatDocModel));
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);

    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pAfter, DKS_RIGHT);
    assert(pAfter != NULL);
    assert(pRightZone != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_window_layout_apply_rolls_back_on_floating_document_restore_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pBefore = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pBefore != NULL);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    assert(!WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        &floatDocModel));
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);

    HWND hWndWorkspaceAfter = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfter == hWndWorkspaceBefore);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pAfter != NULL);
    assert(runtime_find_model_node_by_name(pAfter, L"WorkspaceContainer") != NULL);
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_LEFT), L"Toolbox"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_RIGHT), L"GLWindow"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_RIGHT), L"Palette"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_RIGHT), L"Layers"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_BOTTOM), L"Option Bar"));

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pTarget);
    DockModel_Destroy(pBefore);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_window_layout_apply_rolls_back_on_partial_floating_document_restore_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode floatDocRootA = { 0 };
    DockModelNode floatDocWorkspaceA = { 0 };
    floatDocRootA.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRootA.szName, ARRAYSIZE(floatDocRootA.szName), L"Root");
    floatDocRootA.pChild1 = &floatDocWorkspaceA;
    floatDocWorkspaceA.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspaceA.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspaceA.szName, ARRAYSIZE(floatDocWorkspaceA.szName), L"WorkspaceContainer");

    DockModelNode floatDocRootB = { 0 };
    DockModelNode floatDocWorkspaceB = { 0 };
    floatDocRootB.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRootB.szName, ARRAYSIZE(floatDocRootB.szName), L"Root");
    floatDocRootB.pChild1 = &floatDocWorkspaceB;
    floatDocWorkspaceB.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspaceB.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspaceB.szName, ARRAYSIZE(floatDocWorkspaceB.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 2;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRootA;
    SetRect(&floatDocModel.entries[1].rcWindow, 930, 200, 1250, 560);
    floatDocModel.entries[1].pLayoutModel = &floatDocRootB;

    g_runtimeFloatingDocumentLayoutRestoreFailCountdown = 1;
    PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(runtime_fail_floating_document_layout_restore_on_countdown);
    assert(!WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        NULL,
        &floatDocModel));
    PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(NULL);
    g_runtimeFloatingDocumentLayoutRestoreFailCountdown = -1;

    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pAfter, DKS_RIGHT);
    assert(pAfter != NULL);
    assert(pRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_window_layout_apply_rolls_back_on_floating_tool_restore_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pBefore = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pBefore != NULL);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    PanitentDockFloating_SetRestoreEntryTestHook(runtime_fail_floating_tool_restore);
    assert(!WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pTarget,
        &floatingModel,
        NULL));
    PanitentDockFloating_SetRestoreEntryTestHook(NULL);

    HWND hWndWorkspaceAfter = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfter == hWndWorkspaceBefore);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pAfter != NULL);
    assert(runtime_find_model_node_by_name(pAfter, L"WorkspaceContainer") != NULL);
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_LEFT), L"Toolbox"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_RIGHT), L"GLWindow"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_RIGHT), L"Palette"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_RIGHT), L"Layers"));
    assert(runtime_model_subtree_contains_name(runtime_find_model_zone(pAfter, DKS_BOTTOM), L"Option Bar"));

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pTarget);
    DockModel_Destroy(pBefore);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_document_workspace_model_docking_creates_split_group(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pIncomingWorkspace = WorkspaceContainer_Create();
    assert(pIncomingWorkspace != NULL);
    HWND hWndIncomingWorkspace = Window_CreateWindow((Window*)pIncomingWorkspace, NULL);
    assert(hWndIncomingWorkspace && IsWindow(hWndIncomingWorkspace));
    ShowWindow(hWndIncomingWorkspace, SW_HIDE);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pIncomingWorkspace));

    DockTargetHit targetHit = { 0 };
    targetHit.nDockSide = DKS_RIGHT;
    targetHit.bLocalTarget = TRUE;
    targetHit.hWndAnchor = hWndMainWorkspace;
    assert(DockHostWindow_DockHWNDToTarget(fixture.pDockHostWindow, hWndIncomingWorkspace, &targetHit, 240));

    HWND hWndWorkspaceAfterDock = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterDock == hWndMainWorkspace);
    assert(GetParent(hWndIncomingWorkspace) == fixture.hWndDockHost);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 2);

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pApplied != NULL);
    assert(runtime_count_model_role(pApplied, DOCK_ROLE_WORKSPACE) == 2);

    assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 1);

    DockModel_Destroy(pApplied);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_empty_document_group_cleanup_uses_model_first_remove(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    WorkspaceContainer* pIncomingWorkspace = WorkspaceContainer_Create();
    assert(pIncomingWorkspace != NULL);
    HWND hWndIncomingWorkspace = Window_CreateWindow((Window*)pIncomingWorkspace, NULL);
    assert(hWndIncomingWorkspace && IsWindow(hWndIncomingWorkspace));
    ShowWindow(hWndIncomingWorkspace, SW_HIDE);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pIncomingWorkspace));

    DockTargetHit targetHit = { 0 };
    targetHit.nDockSide = DKS_RIGHT;
    targetHit.bLocalTarget = TRUE;
    targetHit.hWndAnchor = hWndMainWorkspace;
    assert(DockHostWindow_DockHWNDToTarget(fixture.pDockHostWindow, hWndIncomingWorkspace, &targetHit, 240));

    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 2);
    assert(WorkspaceContainer_GetViewportCount(pIncomingWorkspace) == 1);

    WorkspaceContainer_MoveAllViewportsTo(pIncomingWorkspace, pMainWorkspace);

    assert(WorkspaceContainer_GetViewportCount(pIncomingWorkspace) == 0);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 1);
    assert(!IsWindow(hWndIncomingWorkspace));
    assert(IsWindow(hWndMainWorkspace));

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pApplied != NULL);
    assert(runtime_count_model_role(pApplied, DOCK_ROLE_WORKSPACE) == 1);

    DockModel_Destroy(pApplied);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_document_group_undock_to_floating_uses_model_first_remove(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pIncomingWorkspace = WorkspaceContainer_Create();
    assert(pIncomingWorkspace != NULL);
    HWND hWndIncomingWorkspace = Window_CreateWindow((Window*)pIncomingWorkspace, NULL);
    assert(hWndIncomingWorkspace && IsWindow(hWndIncomingWorkspace));
    ShowWindow(hWndIncomingWorkspace, SW_HIDE);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pIncomingWorkspace));

    DockTargetHit targetHit = { 0 };
    targetHit.nDockSide = DKS_RIGHT;
    targetHit.bLocalTarget = TRUE;
    targetHit.hWndAnchor = hWndMainWorkspace;
    assert(DockHostWindow_DockHWNDToTarget(fixture.pDockHostWindow, hWndIncomingWorkspace, &targetHit, 240));

    TreeNode* pIncomingNode = runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndIncomingWorkspace);
    DockData* pIncomingData = pIncomingNode ? (DockData*)pIncomingNode->data : NULL;
    assert(pIncomingNode != NULL);
    assert(pIncomingData != NULL);

    int x = pIncomingData->rc.left + max(1, Win32_Rect_GetWidth(&pIncomingData->rc) / 2);
    int y = pIncomingData->rc.top + 8;

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    DockHostDrag_UndockToFloating(fixture.pDockHostWindow, pIncomingNode, x, y);

    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 1);
    assert(IsWindow(hWndIncomingWorkspace));

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 1);
    assert(counts.nDocumentHosts == 0);

    WCHAR szParentClass[64] = L"";
    HWND hWndParent = GetParent(hWndIncomingWorkspace);
    assert(hWndParent && IsWindow(hWndParent));
    GetClassNameW(hWndParent, szParentClass, ARRAYSIZE(szParentClass));
    assert(wcscmp(szParentClass, L"__FloatingWindowContainer") == 0);

    DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pApplied != NULL);
    assert(runtime_count_model_role(pApplied, DOCK_ROLE_WORKSPACE) == 1);

    DockModel_Destroy(pApplied);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_document_group_undock_rolls_back_on_floating_create_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pIncomingWorkspace = WorkspaceContainer_Create();
    assert(pIncomingWorkspace != NULL);
    HWND hWndIncomingWorkspace = Window_CreateWindow((Window*)pIncomingWorkspace, NULL);
    assert(hWndIncomingWorkspace && IsWindow(hWndIncomingWorkspace));
    ShowWindow(hWndIncomingWorkspace, SW_HIDE);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pIncomingWorkspace));

    DockTargetHit targetHit = { 0 };
    targetHit.nDockSide = DKS_RIGHT;
    targetHit.bLocalTarget = TRUE;
    targetHit.hWndAnchor = hWndMainWorkspace;
    assert(DockHostWindow_DockHWNDToTarget(fixture.pDockHostWindow, hWndIncomingWorkspace, &targetHit, 240));

    TreeNode* pIncomingNode = runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndIncomingWorkspace);
    DockData* pIncomingData = pIncomingNode ? (DockData*)pIncomingNode->data : NULL;
    assert(pIncomingNode != NULL);
    assert(pIncomingData != NULL);

    int x = pIncomingData->rc.left + max(1, Win32_Rect_GetWidth(&pIncomingData->rc) / 2);
    int y = pIncomingData->rc.top + 8;

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 2);

    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    DockHostDrag_UndockToFloating(fixture.pDockHostWindow, pIncomingNode, x, y);
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);

    assert(IsWindow(hWndIncomingWorkspace));
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 2);
    assert(runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndIncomingWorkspace) != NULL);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_single_document_float_uses_document_host_helper(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pMainWorkspace));
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);

    ViewportWindow* pViewport = WorkspaceContainer_GetCurrentViewport(pMainWorkspace);
    assert(pViewport != NULL);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    WorkspaceContainer_FloatViewport(pMainWorkspace, pViewport, 420, 320, FALSE);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 1);
    assert(counts.nDocumentHosts == 0);
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 0);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 1);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_single_document_float_rolls_back_on_floating_create_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pMainWorkspace));
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);

    ViewportWindow* pViewport = WorkspaceContainer_GetCurrentViewport(pMainWorkspace);
    assert(pViewport != NULL);
    HWND hWndViewport = Window_GetHWND((Window*)pViewport);
    assert(hWndViewport && IsWindow(hWndViewport));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    WorkspaceContainer_FloatViewport(pMainWorkspace, pViewport, 420, 320, FALSE);
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);
    assert(WorkspaceContainer_GetCurrentViewport(pMainWorkspace) == pViewport);
    assert(PanitentApp_GetActiveViewport(PanitentApp_Instance()) == pViewport);
    assert(GetParent(hWndViewport) == hWndMainWorkspace);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 1);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_non_current_document_float_failure_preserves_order_and_active_tab(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    Canvas* pCanvasA = Canvas_Create(32, 32);
    Canvas* pCanvasB = Canvas_Create(32, 32);
    assert(pCanvasA != NULL);
    assert(pCanvasB != NULL);

    Document* pDocumentA = Document_CreateWithCanvas(pCanvasA);
    Document* pDocumentB = Document_CreateWithCanvas(pCanvasB);
    assert(pDocumentA != NULL);
    assert(pDocumentB != NULL);
    assert(Document_AttachToWorkspace(pDocumentA, pMainWorkspace));
    assert(Document_AttachToWorkspace(pDocumentB, pMainWorkspace));
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 2);

    ViewportWindow* pViewportA = WorkspaceContainer_GetViewportAt(pMainWorkspace, 0);
    ViewportWindow* pViewportB = WorkspaceContainer_GetViewportAt(pMainWorkspace, 1);
    assert(pViewportA != NULL);
    assert(pViewportB != NULL);
    assert(WorkspaceContainer_GetCurrentViewport(pMainWorkspace) == pViewportB);
    assert(PanitentApp_GetActiveViewport(PanitentApp_Instance()) == pViewportB);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    WorkspaceContainer_FloatViewport(pMainWorkspace, pViewportA, 420, 320, FALSE);
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 2);
    assert(WorkspaceContainer_GetViewportAt(pMainWorkspace, 0) == pViewportA);
    assert(WorkspaceContainer_GetViewportAt(pMainWorkspace, 1) == pViewportB);
    assert(WorkspaceContainer_GetCurrentViewport(pMainWorkspace) == pViewportB);
    assert(PanitentApp_GetActiveViewport(PanitentApp_Instance()) == pViewportB);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_floating_document_session_restore_is_idempotent(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_floating_document_session_file();

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pMainWorkspace));
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);

    ViewportWindow* pViewport = WorkspaceContainer_GetCurrentViewport(pMainWorkspace);
    assert(pViewport != NULL);
    WorkspaceContainer_FloatViewport(pMainWorkspace, pViewport, 420, 320, FALSE);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces + counts.nDocumentHosts == 1);

    assert(PanitentFloatingDocumentSession_Save(fixture.pApp, fixture.pDockHostWindow));
    assert(PanitentFloatingDocumentSession_Restore(fixture.pApp, fixture.pDockHostWindow));

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces + counts.nDocumentHosts == 1);

    runtime_delete_floating_document_session_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_multi_workspace_floating_document_session_restore_is_idempotent(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_floating_document_session_file();

    DockModelNode* pLayout = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayout != NULL);

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocSplit = { 0 };
    DockModelNode floatDocWorkspace1 = { 0 };
    DockModelNode floatDocWorkspace2 = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocSplit;
    floatDocSplit.nRole = DOCK_ROLE_PANEL_SPLIT;
    floatDocSplit.dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
    floatDocSplit.fGripPos = 0.5f;
    wcscpy_s(floatDocSplit.szName, ARRAYSIZE(floatDocSplit.szName), L"DockShell.PanelSplit");
    floatDocSplit.pChild1 = &floatDocWorkspace1;
    floatDocSplit.pChild2 = &floatDocWorkspace2;
    floatDocWorkspace1.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace1.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace1.uNodeId = 301;
    wcscpy_s(floatDocWorkspace1.szName, ARRAYSIZE(floatDocWorkspace1.szName), L"WorkspaceContainer");
    floatDocWorkspace2.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace2.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace2.uNodeId = 302;
    wcscpy_s(floatDocWorkspace2.szName, ARRAYSIZE(floatDocWorkspace2.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLayout, NULL, &floatDocModel));

    HWND hWndFloatingDocument = runtime_find_floating_document_window();
    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    FloatingWindowContainer* pFloatingDocument = (FloatingWindowContainer*)WindowMap_Get(hWndFloatingDocument);
    assert(pFloatingDocument != NULL);
    assert(pFloatingDocument->hWndChild && IsWindow(pFloatingDocument->hWndChild));

    HWND hWorkspaceHwnds[8] = { 0 };
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloatingDocument->hWndChild,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    assert(nWorkspaceCount == 2);

    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWorkspaceHwnds[i]);
        assert(pWorkspace != NULL);
        Canvas* pCanvas = Canvas_Create(32, 32);
        Document* pDocument = Document_CreateWithCanvas(pCanvas);
        assert(pCanvas != NULL);
        assert(pDocument != NULL);
        assert(Document_AttachToWorkspace(pDocument, pWorkspace));
    }

    assert(PanitentFloatingDocumentSession_Save(fixture.pApp, fixture.pDockHostWindow));
    assert(PanitentFloatingDocumentSession_Restore(fixture.pApp, fixture.pDockHostWindow));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces + counts.nDocumentHosts == 1);

    hWndFloatingDocument = runtime_find_floating_document_window();
    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    pFloatingDocument = (FloatingWindowContainer*)WindowMap_Get(hWndFloatingDocument);
    assert(pFloatingDocument != NULL);
    assert(pFloatingDocument->hWndChild && IsWindow(pFloatingDocument->hWndChild));

    memset(hWorkspaceHwnds, 0, sizeof(hWorkspaceHwnds));
    nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloatingDocument->hWndChild,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    assert(nWorkspaceCount == 2);
    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWorkspaceHwnds[i]);
        assert(pWorkspace != NULL);
        assert(WorkspaceContainer_GetViewportCount(pWorkspace) == 1);
    }

    runtime_delete_floating_document_session_file();
    DockModel_Destroy(pLayout);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_multi_workspace_floating_document_layout_restore_is_idempotent(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocSplit = { 0 };
    DockModelNode floatDocWorkspace1 = { 0 };
    DockModelNode floatDocWorkspace2 = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocSplit;
    floatDocSplit.nRole = DOCK_ROLE_PANEL_SPLIT;
    floatDocSplit.dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
    floatDocSplit.fGripPos = 0.5f;
    wcscpy_s(floatDocSplit.szName, ARRAYSIZE(floatDocSplit.szName), L"DockShell.PanelSplit");
    floatDocSplit.pChild1 = &floatDocWorkspace1;
    floatDocSplit.pChild2 = &floatDocWorkspace2;
    floatDocWorkspace1.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace1.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace1.uNodeId = 401;
    wcscpy_s(floatDocWorkspace1.szName, ARRAYSIZE(floatDocWorkspace1.szName), L"WorkspaceContainer");
    floatDocWorkspace2.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace2.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace2.uNodeId = 402;
    wcscpy_s(floatDocWorkspace2.szName, ARRAYSIZE(floatDocWorkspace2.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    assert(PanitentFloatingDocumentLayout_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatDocModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    HWND hWndFloatingDocument = runtime_find_floating_document_window();
    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    FloatingWindowContainer* pFloatingDocument = (FloatingWindowContainer*)WindowMap_Get(hWndFloatingDocument);
    assert(pFloatingDocument != NULL);
    assert(pFloatingDocument->hWndChild && IsWindow(pFloatingDocument->hWndChild));

    HWND hWorkspaceHwnds[8] = { 0 };
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloatingDocument->hWndChild,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    assert(nWorkspaceCount == 2);

    assert(PanitentFloatingDocumentLayout_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatDocModel));

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    hWndFloatingDocument = runtime_find_floating_document_window();
    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    pFloatingDocument = (FloatingWindowContainer*)WindowMap_Get(hWndFloatingDocument);
    assert(pFloatingDocument != NULL);
    assert(pFloatingDocument->hWndChild && IsWindow(pFloatingDocument->hWndChild));

    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWorkspaceHwnds[i]);
        assert(pWorkspace != NULL);
        Canvas* pCanvas = Canvas_Create(32, 32);
        Document* pDocument = Document_CreateWithCanvas(pCanvas);
        assert(pCanvas != NULL);
        assert(pDocument != NULL);
        assert(Document_AttachToWorkspace(pDocument, pWorkspace));
    }

    memset(hWorkspaceHwnds, 0, sizeof(hWorkspaceHwnds));
    nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloatingDocument->hWndChild,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    assert(nWorkspaceCount == 2);

    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWorkspaceHwnds[i]);
        assert(pWorkspace != NULL);
        assert(WorkspaceContainer_GetViewportCount(pWorkspace) == 1);
    }

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_direct_floating_document_layout_restore_strict_mode_rolls_back_on_partial_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pMainWorkspace));
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);

    ViewportWindow* pViewport = WorkspaceContainer_GetCurrentViewport(pMainWorkspace);
    assert(pViewport != NULL);
    WorkspaceContainer_FloatViewport(pMainWorkspace, pViewport, 420, 320, FALSE);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces + counts.nDocumentHosts == 1);

    DockModelNode floatDocRootA = { 0 };
    DockModelNode floatDocWorkspaceA = { 0 };
    floatDocRootA.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRootA.szName, ARRAYSIZE(floatDocRootA.szName), L"Root");
    floatDocRootA.pChild1 = &floatDocWorkspaceA;
    floatDocWorkspaceA.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspaceA.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspaceA.szName, ARRAYSIZE(floatDocWorkspaceA.szName), L"WorkspaceContainer");

    DockModelNode floatDocRootB = { 0 };
    DockModelNode floatDocWorkspaceB = { 0 };
    floatDocRootB.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRootB.szName, ARRAYSIZE(floatDocRootB.szName), L"Root");
    floatDocRootB.pChild1 = &floatDocWorkspaceB;
    floatDocWorkspaceB.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspaceB.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspaceB.szName, ARRAYSIZE(floatDocWorkspaceB.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 2;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRootA;
    SetRect(&floatDocModel.entries[1].rcWindow, 930, 200, 1250, 560);
    floatDocModel.entries[1].pLayoutModel = &floatDocRootB;

    g_runtimeFloatingDocumentLayoutRestoreFailCountdown = 1;
    PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(runtime_fail_floating_document_layout_restore_on_countdown);
    assert(!PanitentFloatingDocumentLayout_RestoreModelEx(fixture.pApp, fixture.pDockHostWindow, &floatDocModel, TRUE));
    PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(NULL);
    g_runtimeFloatingDocumentLayoutRestoreFailCountdown = -1;

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces + counts.nDocumentHosts == 1);

    HWND hWndFloating = runtime_find_floating_document_window();
    assert(hWndFloating && IsWindow(hWndFloating));
    FloatingWindowContainer* pFloating = (FloatingWindowContainer*)WindowMap_Get(hWndFloating);
    assert(pFloating != NULL);
    assert(pFloating->hWndChild && IsWindow(pFloating->hWndChild));
    HWND hWndWorkspaces[4] = { 0 };
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloating->hWndChild,
        hWndWorkspaces,
        ARRAYSIZE(hWndWorkspaces));
    assert(nWorkspaceCount == 1);

    WorkspaceContainer* pFloatingWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndWorkspaces[0]);
    assert(pFloatingWorkspace != NULL);
    assert(WorkspaceContainer_GetViewportCount(pFloatingWorkspace) == 1);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_try_dock_floating_workspace_uses_shared_document_transition(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));
    const UINT IDM_FLOAT_DOCK_RUNTIME = 1001;

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pMainWorkspace));
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);

    ViewportWindow* pViewport = WorkspaceContainer_GetCurrentViewport(pMainWorkspace);
    assert(pViewport != NULL);
    WorkspaceContainer_FloatViewport(pMainWorkspace, pViewport, 420, 320, FALSE);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 1);

    HWND hWndFloatingDocument = NULL;
    EnumWindows(runtime_enum_floating_windows, (LPARAM)&counts);
    for (HWND hWnd = GetTopWindow(NULL); hWnd; hWnd = GetNextWindow(hWnd, GW_HWNDNEXT))
    {
        if (!runtime_is_class_name(hWnd, L"__FloatingWindowContainer"))
        {
            continue;
        }

        FloatingWindowContainer* pFloating = (FloatingWindowContainer*)WindowMap_Get(hWnd);
        if (!pFloating || pFloating->nDockPolicy != FLOAT_DOCK_POLICY_DOCUMENT)
        {
            continue;
        }

        if (pFloating->hWndChild && IsWindow(pFloating->hWndChild))
        {
            hWndFloatingDocument = hWnd;
            break;
        }
    }

    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    SendMessageW(hWndFloatingDocument, WM_COMMAND, IDM_FLOAT_DOCK_RUNTIME, 0);
    assert(WorkspaceContainer_GetViewportCount(pMainWorkspace) == 1);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentWorkspaces == 0);
    assert(counts.nDocumentHosts == 0);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_document_side_dock_failure_rolls_back_floating_document_host_merge(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));
    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    DockModelNode* pLayout = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayout != NULL);

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocSplit = { 0 };
    DockModelNode floatDocWorkspace1 = { 0 };
    DockModelNode floatDocWorkspace2 = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocSplit;
    floatDocSplit.nRole = DOCK_ROLE_PANEL_SPLIT;
    floatDocSplit.dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
    floatDocSplit.fGripPos = 0.5f;
    wcscpy_s(floatDocSplit.szName, ARRAYSIZE(floatDocSplit.szName), L"DockShell.PanelSplit");
    floatDocSplit.pChild1 = &floatDocWorkspace1;
    floatDocSplit.pChild2 = &floatDocWorkspace2;
    floatDocWorkspace1.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace1.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace1.uNodeId = 101;
    wcscpy_s(floatDocWorkspace1.szName, ARRAYSIZE(floatDocWorkspace1.szName), L"WorkspaceContainer");
    floatDocWorkspace2.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace2.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace2.uNodeId = 102;
    wcscpy_s(floatDocWorkspace2.szName, ARRAYSIZE(floatDocWorkspace2.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLayout, NULL, &floatDocModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    HWND hWndFloatingDocument = runtime_find_floating_document_window();
    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    FloatingWindowContainer* pFloatingDocument = (FloatingWindowContainer*)WindowMap_Get(hWndFloatingDocument);
    assert(pFloatingDocument != NULL);
    assert(pFloatingDocument->hWndChild && IsWindow(pFloatingDocument->hWndChild));
    assert(FloatingChildHost_GetKind(pFloatingDocument->hWndChild) == FLOAT_DOCK_CHILD_DOCUMENT_HOST);

    HWND hSourceChildBefore = pFloatingDocument->hWndChild;
    HWND hWorkspaceHwnds[8] = { 0 };
    int nWorkspaceCountBefore = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        hSourceChildBefore,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    assert(nWorkspaceCountBefore == 2);

    for (int i = 0; i < nWorkspaceCountBefore; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWorkspaceHwnds[i]);
        assert(pWorkspace != NULL);
        Canvas* pCanvas = Canvas_Create(32, 32);
        Document* pDocument = Document_CreateWithCanvas(pCanvas);
        assert(pCanvas != NULL);
        assert(pDocument != NULL);
        assert(Document_AttachToWorkspace(pDocument, pWorkspace));
    }

    DocumentDockTransition_SetDockTestHook(runtime_fail_document_dock_once);
    assert(!DocumentDockTransition_DockSourceToWorkspace(
        hWndFloatingDocument,
        &pFloatingDocument->hWndChild,
        pMainWorkspace,
        DKS_RIGHT,
        240));
    DocumentDockTransition_SetDockTestHook(NULL);

    assert(pFloatingDocument->hWndChild == hSourceChildBefore);
    assert(IsWindow(pFloatingDocument->hWndChild));
    assert(FloatingChildHost_GetKind(pFloatingDocument->hWndChild) == FLOAT_DOCK_CHILD_DOCUMENT_HOST);

    HWND hWorkspaceHwndsAfter[8] = { 0 };
    int nWorkspaceCountAfter = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloatingDocument->hWndChild,
        hWorkspaceHwndsAfter,
        ARRAYSIZE(hWorkspaceHwndsAfter));
    assert(nWorkspaceCountAfter == 2);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 1);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pAfter, DKS_RIGHT);
    assert(pAfter != NULL);
    assert(pRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pLayout);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_document_side_dock_merges_floating_document_host_successfully(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));
    WorkspaceContainer* pMainWorkspace = (WorkspaceContainer*)WindowMap_Get(hWndMainWorkspace);
    assert(pMainWorkspace != NULL);

    DockModelNode* pLayout = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayout != NULL);

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocSplit = { 0 };
    DockModelNode floatDocWorkspace1 = { 0 };
    DockModelNode floatDocWorkspace2 = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocSplit;
    floatDocSplit.nRole = DOCK_ROLE_PANEL_SPLIT;
    floatDocSplit.dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
    floatDocSplit.fGripPos = 0.5f;
    wcscpy_s(floatDocSplit.szName, ARRAYSIZE(floatDocSplit.szName), L"DockShell.PanelSplit");
    floatDocSplit.pChild1 = &floatDocWorkspace1;
    floatDocSplit.pChild2 = &floatDocWorkspace2;
    floatDocWorkspace1.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace1.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace1.uNodeId = 201;
    wcscpy_s(floatDocWorkspace1.szName, ARRAYSIZE(floatDocWorkspace1.szName), L"WorkspaceContainer");
    floatDocWorkspace2.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace2.nPaneKind = DOCK_PANE_DOCUMENT;
    floatDocWorkspace2.uNodeId = 202;
    wcscpy_s(floatDocWorkspace2.szName, ARRAYSIZE(floatDocWorkspace2.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLayout, NULL, &floatDocModel));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    HWND hWndFloatingDocument = runtime_find_floating_document_window();
    assert(hWndFloatingDocument && IsWindow(hWndFloatingDocument));
    FloatingWindowContainer* pFloatingDocument = (FloatingWindowContainer*)WindowMap_Get(hWndFloatingDocument);
    assert(pFloatingDocument != NULL);
    assert(pFloatingDocument->hWndChild && IsWindow(pFloatingDocument->hWndChild));
    assert(FloatingChildHost_GetKind(pFloatingDocument->hWndChild) == FLOAT_DOCK_CHILD_DOCUMENT_HOST);

    HWND hWorkspaceHwnds[8] = { 0 };
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(
        pFloatingDocument->hWndChild,
        hWorkspaceHwnds,
        ARRAYSIZE(hWorkspaceHwnds));
    assert(nWorkspaceCount == 2);
    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)WindowMap_Get(hWorkspaceHwnds[i]);
        assert(pWorkspace != NULL);
        Canvas* pCanvas = Canvas_Create(32, 32);
        Document* pDocument = Document_CreateWithCanvas(pCanvas);
        assert(pCanvas != NULL);
        assert(pDocument != NULL);
        assert(Document_AttachToWorkspace(pDocument, pWorkspace));
    }

    assert(DocumentDockTransition_DockSourceToWorkspace(
        hWndFloatingDocument,
        &pFloatingDocument->hWndChild,
        pMainWorkspace,
        DKS_RIGHT,
        240));
    assert(pFloatingDocument->hWndChild == NULL);
    DestroyWindow(hWndFloatingDocument);

    runtime_collect_floating_counts(&counts);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_count_live_role(DockHostWindow_GetRoot(fixture.pDockHostWindow), DOCK_ROLE_WORKSPACE) == 2);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pAfter != NULL);
    assert(runtime_count_model_role(pAfter, DOCK_ROLE_WORKSPACE) == 2);

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pLayout);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_layout_apply_preserves_workspace_binding_by_node_id(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndMainWorkspace = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndMainWorkspace && IsWindow(hWndMainWorkspace));

    WorkspaceContainer* pIncomingWorkspace = WorkspaceContainer_Create();
    assert(pIncomingWorkspace != NULL);
    HWND hWndIncomingWorkspace = Window_CreateWindow((Window*)pIncomingWorkspace, NULL);
    assert(hWndIncomingWorkspace && IsWindow(hWndIncomingWorkspace));
    ShowWindow(hWndIncomingWorkspace, SW_HIDE);

    Canvas* pCanvas = Canvas_Create(32, 32);
    assert(pCanvas != NULL);
    Document* pDocument = Document_CreateWithCanvas(pCanvas);
    assert(pDocument != NULL);
    assert(Document_AttachToWorkspace(pDocument, pIncomingWorkspace));

    DockTargetHit targetHit = { 0 };
    targetHit.nDockSide = DKS_RIGHT;
    targetHit.bLocalTarget = TRUE;
    targetHit.hWndAnchor = hWndMainWorkspace;
    assert(DockHostWindow_DockHWNDToTarget(fixture.pDockHostWindow, hWndIncomingWorkspace, &targetHit, 240));

    TreeNode* pMainWorkspaceNode = runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndMainWorkspace);
    TreeNode* pIncomingWorkspaceNode = runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndIncomingWorkspace);
    DockData* pMainWorkspaceData = pMainWorkspaceNode ? (DockData*)pMainWorkspaceNode->data : NULL;
    DockData* pIncomingWorkspaceData = pIncomingWorkspaceNode ? (DockData*)pIncomingWorkspaceNode->data : NULL;
    assert(pMainWorkspaceData != NULL);
    assert(pIncomingWorkspaceData != NULL);
    assert(pMainWorkspaceData->uModelNodeId != 0);
    assert(pIncomingWorkspaceData->uModelNodeId != 0);

    const uint32_t uMainNodeId = pMainWorkspaceData->uModelNodeId;
    const uint32_t uIncomingNodeId = pIncomingWorkspaceData->uModelNodeId;

    DockModelNode* pCurrentModel = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pCurrentModel != NULL);

    /* Swap the two workspace nodes while keeping their original node ids. */
    DockModelNode* pSwapped = DockModelOps_CloneTree(pCurrentModel);
    assert(pSwapped != NULL);
    DockModelNode* pSplit = pSwapped->pChild1;
    while (pSplit && !(pSplit->nRole == DOCK_ROLE_PANEL_SPLIT &&
        pSplit->pChild1 && pSplit->pChild1->nRole == DOCK_ROLE_WORKSPACE &&
        pSplit->pChild2 && pSplit->pChild2->nRole == DOCK_ROLE_WORKSPACE))
    {
        if (pSplit->pChild1 && runtime_count_model_role(pSplit->pChild1, DOCK_ROLE_WORKSPACE) >= 2)
        {
            pSplit = pSplit->pChild1;
        }
        else if (pSplit->pChild2 && runtime_count_model_role(pSplit->pChild2, DOCK_ROLE_WORKSPACE) >= 2)
        {
            pSplit = pSplit->pChild2;
        }
        else {
            pSplit = NULL;
        }
    }
    assert(pSplit != NULL);

    DockModelNode* pTmp = pSplit->pChild1;
    pSplit->pChild1 = pSplit->pChild2;
    pSplit->pChild2 = pTmp;

    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pSwapped, NULL, NULL));

    pMainWorkspaceNode = runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndMainWorkspace);
    pIncomingWorkspaceNode = runtime_find_live_node_by_hwnd(DockHostWindow_GetRoot(fixture.pDockHostWindow), hWndIncomingWorkspace);
    pMainWorkspaceData = pMainWorkspaceNode ? (DockData*)pMainWorkspaceNode->data : NULL;
    pIncomingWorkspaceData = pIncomingWorkspaceNode ? (DockData*)pIncomingWorkspaceNode->data : NULL;
    assert(pMainWorkspaceData != NULL);
    assert(pIncomingWorkspaceData != NULL);
    assert(pMainWorkspaceData->uModelNodeId == uMainNodeId);
    assert(pIncomingWorkspaceData->uModelNodeId == uIncomingNodeId);

    DockModel_Destroy(pSwapped);
    DockModel_Destroy(pCurrentModel);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_named_layout_profile_switch_with_mixed_floating_arrangement(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uIdA = 0x7B000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    const uint32_t uIdB = uIdA + 1u;
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pLayoutA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingA = { 0 };
    FloatingDocumentLayoutModel floatDocsA = { 0 };
    assert(pLayoutA != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingA));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsA));
    assert(WindowLayoutProfile_SaveBundle(uIdA, pLayoutA, &floatingA, &floatDocsA));

    DockModelNode* pLayoutB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayoutB != NULL);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pLayoutB, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pLayoutB, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingB = { 0 };
    floatingB.nEntries = 1;
    SetRect(&floatingB.entries[0].rcWindow, 180, 140, 500, 470);
    floatingB.entries[0].iDockSizeHint = 240;
    floatingB.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingB.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocsB = { 0 };
    floatDocsB.nEntryCount = 1;
    SetRect(&floatDocsB.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocsB.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutProfile_SaveBundle(uIdB, pLayoutB, &floatingB, &floatDocsB));

    DockModelNode* pLoadedLayout = NULL;
    DockFloatingLayoutFileModel loadedFloating = { 0 };
    FloatingDocumentLayoutModel loadedFloatDocs = { 0 };

    assert(WindowLayoutProfile_LoadBundle(uIdB, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    HWND hWndWorkspaceAfterB = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterB == hWndWorkspaceBefore);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAppliedB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZoneB = runtime_find_model_zone(pAppliedB, DKS_RIGHT);
    assert(pAppliedB != NULL);
    assert(pRightZoneB != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZoneB, L"GLWindow"));

    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);

    pLoadedLayout = NULL;
    memset(&loadedFloating, 0, sizeof(loadedFloating));
    memset(&loadedFloatDocs, 0, sizeof(loadedFloatDocs));
    assert(WindowLayoutProfile_LoadBundle(uIdA, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    HWND hWndWorkspaceAfterA = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterA == hWndWorkspaceBefore);

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAppliedA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZoneA = runtime_find_model_zone(pAppliedA, DKS_RIGHT);
    assert(pAppliedA != NULL);
    assert(pRightZoneA != NULL);
    assert(runtime_model_subtree_contains_name(pRightZoneA, L"GLWindow"));

    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);

    pLoadedLayout = NULL;
    memset(&loadedFloating, 0, sizeof(loadedFloating));
    memset(&loadedFloatDocs, 0, sizeof(loadedFloatDocs));
    assert(WindowLayoutProfile_LoadBundle(uIdB, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));
    assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    HWND hWndWorkspaceAfterB2 = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceAfterB2 == hWndWorkspaceBefore);

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAppliedB2 = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZoneB2 = runtime_find_model_zone(pAppliedB2, DKS_RIGHT);
    assert(pAppliedB2 != NULL);
    assert(pRightZoneB2 != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZoneB2, L"GLWindow"));

    assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
    assert(pReset != NULL);
    assert(pResetRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));

    DockModel_Destroy(pReset);
    DockModel_Destroy(pAppliedB2);
    DockModel_Destroy(pAppliedA);
    DockModel_Destroy(pAppliedB);
    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);
    DockModel_Destroy(pLayoutB);
    DockModel_Destroy(pLayoutA);
    DockFloatingLayout_Destroy(&floatingA);
    FloatingDocumentLayoutModel_Destroy(&floatDocsA);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_menu_command_applies_named_mixed_layout_profiles(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uIdA = 0x7C000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    const uint32_t uIdB = uIdA + 1u;
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pLayoutA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingA = { 0 };
    FloatingDocumentLayoutModel floatDocsA = { 0 };
    assert(pLayoutA != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingA));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsA));
    assert(WindowLayoutProfile_SaveBundle(uIdA, pLayoutA, &floatingA, &floatDocsA));

    DockModelNode* pLayoutB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayoutB != NULL);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pLayoutB, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pLayoutB, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingB = { 0 };
    floatingB.nEntries = 1;
    SetRect(&floatingB.entries[0].rcWindow, 180, 140, 500, 470);
    floatingB.entries[0].iDockSizeHint = 240;
    floatingB.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingB.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocsB = { 0 };
    floatDocsB.nEntryCount = 1;
    SetRect(&floatDocsB.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocsB.entries[0].pLayoutModel = &floatDocRoot;
    assert(WindowLayoutProfile_SaveBundle(uIdB, pLayoutB, &floatingB, &floatDocsB));

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, uIdA, L"Layout A"));
    assert(WindowLayoutCatalog_Add(&catalog, uIdB, L"Layout B"));
    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));
    free(pszCatalogPath);

    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 1));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 0));

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    DockModelNode* pAppliedA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZoneA = runtime_find_model_zone(pAppliedA, DKS_RIGHT);
    assert(pAppliedA != NULL);
    assert(pRightZoneA != NULL);
    assert(runtime_model_subtree_contains_name(pRightZoneA, L"GLWindow"));

    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 1));
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_RESET_LAYOUT));
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
    assert(pReset != NULL);
    assert(pResetRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));

    DockModel_Destroy(pAppliedA);
    DockModel_Destroy(pReset);
    DockModel_Destroy(pLayoutB);
    DockModel_Destroy(pLayoutA);
    DockFloatingLayout_Destroy(&floatingA);
    DockFloatingLayout_Destroy(&floatingB);
    FloatingDocumentLayoutModel_Destroy(&floatDocsA);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_menu_command_saves_and_overwrites_named_layout(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();
    runtime_delete_profile_bundle(1);

    wcscpy_s(g_runtimeWindowLayoutPromptName, ARRAYSIZE(g_runtimeWindowLayoutPromptName), L"Layout A");
    WindowLayoutManager_SetPromptSink(runtime_capture_window_layout_prompt);
    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_SAVE_LAYOUT));
    WindowLayoutManager_SetPromptSink(NULL);
    g_runtimeWindowLayoutPromptName[0] = L'\0';

    WindowLayoutCatalog catalog = { 0 };
    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &catalog, NULL));
    assert(catalog.nEntryCount == 1);
    assert(wcscmp(catalog.entries[0].szName, L"Layout A") == 0);
    const uint32_t uIdA = catalog.entries[0].uId;
    free(pszCatalogPath);

    DockModelNode* pMixedLayout = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pMixedLayout != NULL);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pMixedLayout, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pMixedLayout, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel mixedFloating = { 0 };
    mixedFloating.nEntries = 1;
    SetRect(&mixedFloating.entries[0].rcWindow, 180, 140, 500, 470);
    mixedFloating.entries[0].iDockSizeHint = 240;
    mixedFloating.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    mixedFloating.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel mixedFloatDocs = { 0 };
    mixedFloatDocs.nEntryCount = 1;
    SetRect(&mixedFloatDocs.entries[0].rcWindow, 560, 180, 940, 620);
    mixedFloatDocs.entries[0].pLayoutModel = &floatDocRoot;

    assert(WindowLayoutManager_ApplyLayoutBundle(
        &fixture.panitentWindow,
        pMixedLayout,
        &mixedFloating,
        &mixedFloatDocs));

    wcscpy_s(g_runtimeWindowLayoutPromptName, ARRAYSIZE(g_runtimeWindowLayoutPromptName), L"Layout A");
    WindowLayoutManager_SetPromptSink(runtime_capture_window_layout_prompt);
    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_SAVE_LAYOUT));
    WindowLayoutManager_SetPromptSink(NULL);
    g_runtimeWindowLayoutPromptName[0] = L'\0';

    pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &catalog, NULL));
    assert(catalog.nEntryCount == 1);
    assert(catalog.entries[0].uId == uIdA);
    free(pszCatalogPath);

    DockModelNode* pLoadedLayout = NULL;
    DockFloatingLayoutFileModel loadedFloating = { 0 };
    FloatingDocumentLayoutModel loadedFloatDocs = { 0 };
    assert(WindowLayoutProfile_LoadBundle(uIdA, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));
    assert(pLoadedLayout != NULL);
    assert(loadedFloating.nEntries == 1);
    assert(loadedFloatDocs.nEntryCount == 1);
    DockModelNode* pRightZone = runtime_find_model_zone(pLoadedLayout, DKS_RIGHT);
    assert(pRightZone != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);
    DockModel_Destroy(pMixedLayout);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_menu_command_save_catalog_failure_removes_new_bundle(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();
    runtime_delete_profile_bundle(1);

    wcscpy_s(g_runtimeWindowLayoutPromptName, ARRAYSIZE(g_runtimeWindowLayoutPromptName), L"Broken Layout");
    g_runtimeWindowLayoutMessageCount = 0;
    WindowLayoutManager_SetPromptSink(runtime_capture_window_layout_prompt);
    WindowLayoutManager_SetSaveCatalogSink(runtime_fail_window_layout_save_catalog);
    WindowLayoutManager_SetMessageSink(runtime_capture_window_layout_message);
    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_SAVE_LAYOUT));
    WindowLayoutManager_SetPromptSink(NULL);
    WindowLayoutManager_SetSaveCatalogSink(NULL);
    WindowLayoutManager_SetMessageSink(NULL);
    g_runtimeWindowLayoutPromptName[0] = L'\0';

    assert(g_runtimeWindowLayoutMessageCount == 1);

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    WindowLayoutCatalog catalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    BOOL bLoaded = WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &catalog, &status);
    free(pszCatalogPath);
    assert(!bLoaded);
    assert(status == PERSIST_LOAD_NOT_FOUND);

    DockModelNode* pLoadedLayout = NULL;
    DockFloatingLayoutFileModel loadedFloating = { 0 };
    FloatingDocumentLayoutModel loadedFloatDocs = { 0 };
    assert(!WindowLayoutProfile_LoadBundle(1, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_menu_command_save_failure_does_not_persist_catalog_entry(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();

    wcscpy_s(g_runtimeWindowLayoutPromptName, ARRAYSIZE(g_runtimeWindowLayoutPromptName), L"Broken Layout");
    g_runtimeWindowLayoutMessageCount = 0;
    WindowLayoutManager_SetPromptSink(runtime_capture_window_layout_prompt);
    WindowLayoutManager_SetSaveProfileSink(runtime_fail_window_layout_save_profile);
    WindowLayoutManager_SetMessageSink(runtime_capture_window_layout_message);
    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_SAVE_LAYOUT));
    WindowLayoutManager_SetPromptSink(NULL);
    WindowLayoutManager_SetSaveProfileSink(NULL);
    WindowLayoutManager_SetMessageSink(NULL);
    g_runtimeWindowLayoutPromptName[0] = L'\0';

    assert(g_runtimeWindowLayoutMessageCount == 1);

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    WindowLayoutCatalog catalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    BOOL bLoaded = WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &catalog, &status);
    free(pszCatalogPath);
    assert(!bLoaded);
    assert(status == PERSIST_LOAD_NOT_FOUND);

    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_catalog_delete_rolls_back_on_catalog_save_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uId = 0x83000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    runtime_delete_profile_bundle(uId);
    runtime_delete_window_layout_catalog_file();

    DockModelNode* pLayout = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingModel = { 0 };
    FloatingDocumentLayoutModel floatDocModel = { 0 };
    assert(pLayout != NULL);
    assert(WindowLayoutProfile_SaveBundle(uId, pLayout, &floatingModel, &floatDocModel));

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, uId, L"Delete Me"));

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));

    WindowLayoutManager_SetSaveCatalogSink(runtime_fail_window_layout_save_catalog);
    assert(!WindowLayoutManager_DeleteCatalogEntry(&fixture.panitentWindow, &catalog, 0));
    WindowLayoutManager_SetSaveCatalogSink(NULL);

    assert(catalog.nEntryCount == 1);
    assert(catalog.entries[0].uId == uId);

    WindowLayoutCatalog loadedCatalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &loadedCatalog, &status));
    assert(status == PERSIST_LOAD_OK);
    assert(loadedCatalog.nEntryCount == 1);
    assert(loadedCatalog.entries[0].uId == uId);
    free(pszCatalogPath);

    DockModelNode* pLoadedLayout = NULL;
    DockFloatingLayoutFileModel loadedFloating = { 0 };
    FloatingDocumentLayoutModel loadedFloatDocs = { 0 };
    assert(WindowLayoutProfile_LoadBundle(uId, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    DockModel_Destroy(pLoadedLayout);
    DockFloatingLayout_Destroy(&loadedFloating);
    FloatingDocumentLayoutModel_Destroy(&loadedFloatDocs);
    DockModel_Destroy(pLayout);
    runtime_delete_profile_bundle(uId);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_catalog_move_rolls_back_on_catalog_save_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, 1, L"Layout A"));
    assert(WindowLayoutCatalog_Add(&catalog, 2, L"Layout B"));

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));

    WindowLayoutManager_SetSaveCatalogSink(runtime_fail_window_layout_save_catalog);
    assert(!WindowLayoutManager_MoveCatalogEntry(&fixture.panitentWindow, &catalog, 0, 1));
    WindowLayoutManager_SetSaveCatalogSink(NULL);

    assert(catalog.nEntryCount == 2);
    assert(catalog.entries[0].uId == 1);
    assert(catalog.entries[1].uId == 2);

    WindowLayoutCatalog loadedCatalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &loadedCatalog, &status));
    assert(status == PERSIST_LOAD_OK);
    assert(loadedCatalog.nEntryCount == 2);
    assert(loadedCatalog.entries[0].uId == 1);
    assert(loadedCatalog.entries[1].uId == 2);

    free(pszCatalogPath);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_catalog_rename_rolls_back_on_catalog_save_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, 1, L"Layout A"));

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));

    WindowLayoutManager_SetSaveCatalogSink(runtime_fail_window_layout_save_catalog);
    assert(!WindowLayoutManager_RenameCatalogEntry(&fixture.panitentWindow, &catalog, 0, L"Layout B"));
    WindowLayoutManager_SetSaveCatalogSink(NULL);

    assert(catalog.nEntryCount == 1);
    assert(wcscmp(catalog.entries[0].szName, L"Layout A") == 0);

    WindowLayoutCatalog loadedCatalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &loadedCatalog, &status));
    assert(status == PERSIST_LOAD_OK);
    assert(loadedCatalog.nEntryCount == 1);
    assert(wcscmp(loadedCatalog.entries[0].szName, L"Layout A") == 0);

    free(pszCatalogPath);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_repeated_menu_command_mixed_layout_cycles_remain_stable(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uIdA = 0x85000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    const uint32_t uIdB = uIdA + 1u;
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pLayoutA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingA = { 0 };
    FloatingDocumentLayoutModel floatDocsA = { 0 };
    assert(pLayoutA != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingA));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsA));
    assert(WindowLayoutProfile_SaveBundle(uIdA, pLayoutA, &floatingA, &floatDocsA));

    DockModelNode* pLayoutB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayoutB != NULL);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pLayoutB, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pLayoutB, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingB = { 0 };
    floatingB.nEntries = 1;
    SetRect(&floatingB.entries[0].rcWindow, 180, 140, 500, 470);
    floatingB.entries[0].iDockSizeHint = 240;
    floatingB.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingB.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocsB = { 0 };
    floatDocsB.nEntryCount = 1;
    SetRect(&floatDocsB.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocsB.entries[0].pLayoutModel = &floatDocRoot;
    assert(WindowLayoutProfile_SaveBundle(uIdB, pLayoutB, &floatingB, &floatDocsB));

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, uIdA, L"Layout A"));
    assert(WindowLayoutCatalog_Add(&catalog, uIdB, L"Layout B"));
    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));
    free(pszCatalogPath);

    for (int i = 0; i < 3; ++i)
    {
        assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 1));

        FloatingCountContext counts = { 0 };
        runtime_collect_floating_counts(&counts);
        assert(counts.nToolPanels == 1);
        assert(counts.nToolHosts == 0);
        assert(counts.nDocumentHosts == 1);
        assert(counts.nDocumentWorkspaces == 0);
        assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

        DockModelNode* pAppliedB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
        DockModelNode* pRightZoneB = runtime_find_model_zone(pAppliedB, DKS_RIGHT);
        assert(pAppliedB != NULL);
        assert(pRightZoneB != NULL);
        assert(!runtime_model_subtree_contains_name(pRightZoneB, L"GLWindow"));
        DockModel_Destroy(pAppliedB);

        assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 0));

        runtime_collect_floating_counts(&counts);
        assert(counts.nToolPanels == 0);
        assert(counts.nToolHosts == 0);
        assert(counts.nDocumentHosts == 0);
        assert(counts.nDocumentWorkspaces == 0);
        assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

        DockModelNode* pAppliedA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
        DockModelNode* pRightZoneA = runtime_find_model_zone(pAppliedA, DKS_RIGHT);
        assert(pAppliedA != NULL);
        assert(pRightZoneA != NULL);
        assert(runtime_model_subtree_contains_name(pRightZoneA, L"GLWindow"));
        DockModel_Destroy(pAppliedA);
    }

    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_RESET_LAYOUT));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
    assert(pReset != NULL);
    assert(pResetRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));

    DockModel_Destroy(pReset);
    DockModel_Destroy(pLayoutB);
    DockModel_Destroy(pLayoutA);
    DockFloatingLayout_Destroy(&floatingA);
    DockFloatingLayout_Destroy(&floatingB);
    FloatingDocumentLayoutModel_Destroy(&floatDocsA);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_repeated_mixed_layout_cycles_remain_stable(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 140, 160, 440, 480);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 520, 180, 900, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    for (int i = 0; i < 5; ++i)
    {
        assert(WindowLayoutManager_ApplyLayoutBundle(
            &fixture.panitentWindow,
            pTarget,
            &floatingModel,
            &floatDocModel));

        FloatingCountContext counts = { 0 };
        runtime_collect_floating_counts(&counts);
        assert(counts.nToolPanels == 1);
        assert(counts.nToolHosts == 0);
        assert(counts.nDocumentHosts == 1);
        assert(counts.nDocumentWorkspaces == 0);
        assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

        DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
        DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
        assert(pApplied != NULL);
        assert(pRightZone != NULL);
        assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));
        DockModel_Destroy(pApplied);

        assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));

        runtime_collect_floating_counts(&counts);
        assert(counts.nToolPanels == 0);
        assert(counts.nToolHosts == 0);
        assert(counts.nDocumentHosts == 0);
        assert(counts.nDocumentWorkspaces == 0);
        assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

        DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
        DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
        assert(pReset != NULL);
        assert(pResetRightZone != NULL);
        assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));
        DockModel_Destroy(pReset);
    }

    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_repeated_direct_floating_restore_cycles_remain_stable(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pTarget = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pTarget != NULL);

    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pTarget, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pTarget, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingModel = { 0 };
    floatingModel.nEntries = 1;
    SetRect(&floatingModel.entries[0].rcWindow, 180, 140, 500, 470);
    floatingModel.entries[0].iDockSizeHint = 240;
    floatingModel.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingModel.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocModel = { 0 };
    floatDocModel.nEntryCount = 1;
    SetRect(&floatDocModel.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocModel.entries[0].pLayoutModel = &floatDocRoot;

    for (int i = 0; i < 5; ++i)
    {
        assert(WindowLayoutManager_ApplyLayoutBundle(&fixture.panitentWindow, pTarget, NULL, NULL));
        assert(PanitentDockFloating_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatingModel));
        assert(PanitentFloatingDocumentLayout_RestoreModel(fixture.pApp, fixture.pDockHostWindow, &floatDocModel));

        FloatingCountContext counts = { 0 };
        runtime_collect_floating_counts(&counts);
        assert(counts.nToolPanels == 1);
        assert(counts.nToolHosts == 0);
        assert(counts.nDocumentHosts == 1);
        assert(counts.nDocumentWorkspaces == 0);
        assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

        DockModelNode* pApplied = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
        DockModelNode* pRightZone = runtime_find_model_zone(pApplied, DKS_RIGHT);
        assert(pApplied != NULL);
        assert(pRightZone != NULL);
        assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));
        DockModel_Destroy(pApplied);

        assert(WindowLayoutManager_ApplyDefaultLayout(&fixture.panitentWindow));

        runtime_collect_floating_counts(&counts);
        assert(counts.nToolPanels == 0);
        assert(counts.nToolHosts == 0);
        assert(counts.nDocumentHosts == 0);
        assert(counts.nDocumentWorkspaces == 0);
        assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

        DockModelNode* pReset = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
        DockModelNode* pResetRightZone = runtime_find_model_zone(pReset, DKS_RIGHT);
        assert(pReset != NULL);
        assert(pResetRightZone != NULL);
        assert(runtime_model_subtree_contains_name(pResetRightZone, L"GLWindow"));
        DockModel_Destroy(pReset);
    }

    DockModel_Destroy(pTarget);
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_catalog_move_persists_order_on_success(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, 1, L"Layout A"));
    assert(WindowLayoutCatalog_Add(&catalog, 2, L"Layout B"));
    assert(WindowLayoutCatalog_Add(&catalog, 3, L"Layout C"));

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));

    assert(WindowLayoutManager_MoveCatalogEntry(&fixture.panitentWindow, &catalog, 2, 0));
    assert(catalog.nEntryCount == 3);
    assert(catalog.entries[0].uId == 3);
    assert(catalog.entries[1].uId == 1);
    assert(catalog.entries[2].uId == 2);

    WindowLayoutCatalog loadedCatalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &loadedCatalog, &status));
    assert(status == PERSIST_LOAD_OK);
    assert(loadedCatalog.nEntryCount == 3);
    assert(loadedCatalog.entries[0].uId == 3);
    assert(loadedCatalog.entries[1].uId == 1);
    assert(loadedCatalog.entries[2].uId == 2);

    free(pszCatalogPath);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_catalog_rename_persists_name_on_success(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    runtime_delete_window_layout_catalog_file();

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, 1, L"Layout A"));

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));

    assert(WindowLayoutManager_RenameCatalogEntry(&fixture.panitentWindow, &catalog, 0, L"Layout Renamed"));
    assert(catalog.nEntryCount == 1);
    assert(wcscmp(catalog.entries[0].szName, L"Layout Renamed") == 0);

    WindowLayoutCatalog loadedCatalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &loadedCatalog, &status));
    assert(status == PERSIST_LOAD_OK);
    assert(loadedCatalog.nEntryCount == 1);
    assert(wcscmp(loadedCatalog.entries[0].szName, L"Layout Renamed") == 0);

    free(pszCatalogPath);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_catalog_delete_removes_bundle_on_success(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uId = 0x84000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    runtime_delete_profile_bundle(uId);
    runtime_delete_window_layout_catalog_file();

    DockModelNode* pLayout = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingModel = { 0 };
    FloatingDocumentLayoutModel floatDocModel = { 0 };
    assert(pLayout != NULL);
    assert(WindowLayoutProfile_SaveBundle(uId, pLayout, &floatingModel, &floatDocModel));

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, uId, L"Delete Me"));

    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));

    assert(WindowLayoutManager_DeleteCatalogEntry(&fixture.panitentWindow, &catalog, 0));
    assert(catalog.nEntryCount == 0);

    WindowLayoutCatalog loadedCatalog = { 0 };
    PersistLoadStatus status = PERSIST_LOAD_IO_ERROR;
    assert(WindowLayoutCatalog_LoadFromFile(pszCatalogPath, &loadedCatalog, &status));
    assert(status == PERSIST_LOAD_OK);
    assert(loadedCatalog.nEntryCount == 0);
    free(pszCatalogPath);

    DockModelNode* pLoadedLayout = NULL;
    DockFloatingLayoutFileModel loadedFloating = { 0 };
    FloatingDocumentLayoutModel loadedFloatDocs = { 0 };
    assert(!WindowLayoutProfile_LoadBundle(uId, &pLoadedLayout, &loadedFloating, &loadedFloatDocs));

    DockModel_Destroy(pLayout);
    runtime_delete_profile_bundle(uId);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_menu_command_apply_failure_rolls_back_from_mixed_state(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uIdA = 0x7E000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    const uint32_t uIdB = uIdA + 1u;
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pLayoutA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingA = { 0 };
    FloatingDocumentLayoutModel floatDocsA = { 0 };
    assert(pLayoutA != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingA));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsA));
    assert(WindowLayoutProfile_SaveBundle(uIdA, pLayoutA, &floatingA, &floatDocsA));

    DockModelNode* pLayoutB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayoutB != NULL);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pLayoutB, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pLayoutB, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingB = { 0 };
    floatingB.nEntries = 1;
    SetRect(&floatingB.entries[0].rcWindow, 180, 140, 500, 470);
    floatingB.entries[0].iDockSizeHint = 240;
    floatingB.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingB.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocsB = { 0 };
    floatDocsB.nEntryCount = 1;
    SetRect(&floatDocsB.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocsB.entries[0].pLayoutModel = &floatDocRoot;
    assert(WindowLayoutProfile_SaveBundle(uIdB, pLayoutB, &floatingB, &floatDocsB));

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, uIdA, L"Layout A"));
    assert(WindowLayoutCatalog_Add(&catalog, uIdB, L"Layout B"));
    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));
    free(pszCatalogPath);

    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 1));

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    g_runtimeWindowLayoutMessageCount = 0;
    WindowLayoutManager_SetMessageSink(runtime_capture_window_layout_message);
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 1));
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);
    WindowLayoutManager_SetMessageSink(NULL);

    assert(g_runtimeWindowLayoutMessageCount == 1);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 1);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 1);
    assert(counts.nDocumentWorkspaces == 0);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pAfter, DKS_RIGHT);
    assert(pAfter != NULL);
    assert(pRightZone != NULL);
    assert(!runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pLayoutB);
    DockModel_Destroy(pLayoutA);
    DockFloatingLayout_Destroy(&floatingA);
    DockFloatingLayout_Destroy(&floatingB);
    FloatingDocumentLayoutModel_Destroy(&floatDocsA);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

static int test_runtime_menu_command_apply_rolls_back_on_failure(void)
{
    DockRuntimeFixture fixture = { 0 };
    assert(runtime_fixture_init(&fixture));

    const uint32_t uIdA = 0x7D000000u | (uint32_t)(GetCurrentProcessId() & 0x0000FFFFu);
    const uint32_t uIdB = uIdA + 1u;
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();

    HWND hWndWorkspaceBefore = runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer");
    assert(hWndWorkspaceBefore && IsWindow(hWndWorkspaceBefore));

    DockModelNode* pLayoutA = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockFloatingLayoutFileModel floatingA = { 0 };
    FloatingDocumentLayoutModel floatDocsA = { 0 };
    assert(pLayoutA != NULL);
    assert(PanitentDockFloating_CaptureModel(fixture.pApp, fixture.pDockHostWindow, &floatingA));
    assert(PanitentFloatingDocumentLayout_CaptureModel(fixture.pApp, &floatDocsA));
    assert(WindowLayoutProfile_SaveBundle(uIdA, pLayoutA, &floatingA, &floatDocsA));

    DockModelNode* pLayoutB = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    assert(pLayoutB != NULL);
    DockModelNode* pGLWindow = runtime_find_model_node_by_name(pLayoutB, L"GLWindow");
    assert(pGLWindow != NULL);
    assert(DockModelOps_RemoveNodeById(&pLayoutB, pGLWindow->uNodeId));

    DockFloatingLayoutFileModel floatingB = { 0 };
    floatingB.nEntries = 1;
    SetRect(&floatingB.entries[0].rcWindow, 180, 140, 500, 470);
    floatingB.entries[0].iDockSizeHint = 240;
    floatingB.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
    floatingB.entries[0].nViewId = PNT_DOCK_VIEW_GLWINDOW;

    DockModelNode floatDocRoot = { 0 };
    DockModelNode floatDocWorkspace = { 0 };
    floatDocRoot.nRole = DOCK_ROLE_ROOT;
    wcscpy_s(floatDocRoot.szName, ARRAYSIZE(floatDocRoot.szName), L"Root");
    floatDocRoot.pChild1 = &floatDocWorkspace;
    floatDocWorkspace.nRole = DOCK_ROLE_WORKSPACE;
    floatDocWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
    wcscpy_s(floatDocWorkspace.szName, ARRAYSIZE(floatDocWorkspace.szName), L"WorkspaceContainer");

    FloatingDocumentLayoutModel floatDocsB = { 0 };
    floatDocsB.nEntryCount = 1;
    SetRect(&floatDocsB.entries[0].rcWindow, 560, 180, 940, 620);
    floatDocsB.entries[0].pLayoutModel = &floatDocRoot;
    assert(WindowLayoutProfile_SaveBundle(uIdB, pLayoutB, &floatingB, &floatDocsB));

    WindowLayoutCatalog catalog = { 0 };
    WindowLayoutCatalog_Init(&catalog);
    assert(WindowLayoutCatalog_Add(&catalog, uIdA, L"Layout A"));
    assert(WindowLayoutCatalog_Add(&catalog, uIdB, L"Layout B"));
    PTSTR pszCatalogPath = NULL;
    GetAppDataFilePath(L"windowlayouts.dat", &pszCatalogPath);
    assert(pszCatalogPath != NULL);
    assert(WindowLayoutCatalog_SaveToFile(&catalog, pszCatalogPath));
    free(pszCatalogPath);

    g_runtimeWindowLayoutMessageCount = 0;
    WindowLayoutManager_SetMessageSink(runtime_capture_window_layout_message);
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(runtime_fail_floating_document_create);
    assert(WindowLayoutManager_HandleCommand(&fixture.panitentWindow, IDM_WINDOW_APPLY_LAYOUT_BASE + 1));
    FloatingDocumentHost_SetCreatePinnedWindowTestHook(NULL);
    WindowLayoutManager_SetMessageSink(NULL);

    assert(g_runtimeWindowLayoutMessageCount == 1);

    FloatingCountContext counts = { 0 };
    runtime_collect_floating_counts(&counts);
    assert(counts.nToolPanels == 0);
    assert(counts.nToolHosts == 0);
    assert(counts.nDocumentHosts == 0);
    assert(counts.nDocumentWorkspaces == 0);
    assert(runtime_get_live_hwnd_by_name(fixture.pDockHostWindow, L"WorkspaceContainer") == hWndWorkspaceBefore);

    DockModelNode* pAfter = DockModel_CaptureHostLayout(fixture.pDockHostWindow);
    DockModelNode* pRightZone = runtime_find_model_zone(pAfter, DKS_RIGHT);
    assert(pAfter != NULL);
    assert(pRightZone != NULL);
    assert(runtime_model_subtree_contains_name(pRightZone, L"GLWindow"));

    DockModel_Destroy(pAfter);
    DockModel_Destroy(pLayoutB);
    DockModel_Destroy(pLayoutA);
    DockFloatingLayout_Destroy(&floatingA);
    DockFloatingLayout_Destroy(&floatingB);
    FloatingDocumentLayoutModel_Destroy(&floatDocsA);
    runtime_delete_profile_bundle(uIdA);
    runtime_delete_profile_bundle(uIdB);
    runtime_delete_window_layout_catalog_file();
    runtime_fixture_destroy(&fixture);
    return 0;
}

int main(void)
{
    HRESULT hrOle = OleInitialize(NULL);
    BOOL bOleInitialized = SUCCEEDED(hrOle);
    int failed = 0;

    failed |= test_runtime_tool_model_docking_moves_panel_between_zones();
    failed |= test_runtime_window_layout_apply_invalid_bundle_rolls_back();
    failed |= test_runtime_window_layout_apply_rolls_back_on_floating_tool_restore_failure();
    failed |= test_runtime_window_layout_apply_rolls_back_on_floating_document_restore_failure();
    failed |= test_runtime_window_layout_apply_rolls_back_on_partial_floating_document_restore_failure();
    failed |= test_runtime_window_layout_apply_and_reset_preserves_workspace();
    failed |= test_runtime_named_layout_profile_switch_round_trip();
    failed |= test_runtime_apply_mixed_floating_layout_bundle();
    failed |= test_runtime_direct_floating_tool_restore_is_idempotent();
    failed |= test_runtime_direct_floating_tool_host_restore_is_idempotent();
    failed |= test_runtime_direct_floating_tool_restore_strict_mode_rolls_back_on_partial_failure();
    failed |= test_runtime_reapply_mixed_floating_layout_bundle_is_idempotent();
    failed |= test_runtime_repeated_mixed_layout_cycles_remain_stable();
    failed |= test_runtime_repeated_direct_floating_restore_cycles_remain_stable();
    failed |= test_runtime_reapply_mixed_layout_failure_rolls_back_to_existing_mixed_state();
    failed |= test_runtime_named_layout_profile_switch_with_mixed_floating_arrangement();
    failed |= test_runtime_menu_command_applies_named_mixed_layout_profiles();
    failed |= test_runtime_repeated_menu_command_mixed_layout_cycles_remain_stable();
    failed |= test_runtime_menu_command_apply_rolls_back_on_failure();
    failed |= test_runtime_menu_command_apply_failure_rolls_back_from_mixed_state();
    failed |= test_runtime_menu_command_saves_and_overwrites_named_layout();
    failed |= test_runtime_menu_command_save_failure_does_not_persist_catalog_entry();
    failed |= test_runtime_menu_command_save_catalog_failure_removes_new_bundle();
    failed |= test_runtime_catalog_delete_rolls_back_on_catalog_save_failure();
    failed |= test_runtime_catalog_move_rolls_back_on_catalog_save_failure();
    failed |= test_runtime_catalog_rename_rolls_back_on_catalog_save_failure();
    failed |= test_runtime_catalog_move_persists_order_on_success();
    failed |= test_runtime_catalog_rename_persists_name_on_success();
    failed |= test_runtime_catalog_delete_removes_bundle_on_success();
    failed |= test_runtime_document_workspace_model_docking_creates_split_group();
    failed |= test_runtime_empty_document_group_cleanup_uses_model_first_remove();
    failed |= test_runtime_document_group_undock_to_floating_uses_model_first_remove();
    failed |= test_runtime_document_group_undock_rolls_back_on_floating_create_failure();
    failed |= test_runtime_single_document_float_uses_document_host_helper();
    failed |= test_runtime_single_document_float_rolls_back_on_floating_create_failure();
    failed |= test_runtime_non_current_document_float_failure_preserves_order_and_active_tab();
    failed |= test_runtime_floating_document_session_restore_is_idempotent();
    failed |= test_runtime_multi_workspace_floating_document_session_restore_is_idempotent();
    failed |= test_runtime_multi_workspace_floating_document_layout_restore_is_idempotent();
    failed |= test_runtime_direct_floating_document_layout_restore_strict_mode_rolls_back_on_partial_failure();
    failed |= test_runtime_try_dock_floating_workspace_uses_shared_document_transition();
    failed |= test_runtime_document_side_dock_merges_floating_document_host_successfully();
    failed |= test_runtime_document_side_dock_failure_rolls_back_floating_document_host_merge();
    failed |= test_runtime_layout_apply_preserves_workspace_binding_by_node_id();

    if (bOleInitialized)
    {
        OleUninitialize();
    }

    if (failed)
    {
        printf("dockruntime tests FAILED\n");
        return 1;
    }

    printf("dockruntime tests PASSED\n");
    return 0;
}

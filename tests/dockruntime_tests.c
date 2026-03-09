#include <assert.h>
#include <ole2.h>

#include "../src/dockhost.h"
#include "../src/dockhostmodelapply.h"
#include "../src/dockfloatingmodel.h"
#include "../src/dockfloatingpersist.h"
#include "../src/floatingdocumentlayoutmodel.h"
#include "../src/floatingdocumentlayoutpersist.h"
#include "../src/floatingwindowcontainer.h"
#include "../src/dockmodel.h"
#include "../src/dockmodelops.h"
#include "../src/dockshell.h"
#include "../src/panitentapp.h"
#include "../src/panitentwindow.h"
#include "../src/windowlayoutmanager.h"
#include "../src/windowlayoutprofile.h"
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
    runtime_assert_model_equal(pAfter, pBefore);

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

int main(void)
{
    HRESULT hrOle = OleInitialize(NULL);
    BOOL bOleInitialized = SUCCEEDED(hrOle);
    int failed = 0;

    failed |= test_runtime_tool_model_docking_moves_panel_between_zones();
    failed |= test_runtime_window_layout_apply_invalid_bundle_rolls_back();
    failed |= test_runtime_window_layout_apply_and_reset_preserves_workspace();
    failed |= test_runtime_named_layout_profile_switch_round_trip();
    failed |= test_runtime_apply_mixed_floating_layout_bundle();
    failed |= test_runtime_named_layout_profile_switch_with_mixed_floating_arrangement();

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

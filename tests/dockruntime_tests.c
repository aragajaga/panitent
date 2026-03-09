#include <assert.h>
#include <ole2.h>

#include "../src/dockhost.h"
#include "../src/dockhostmodelapply.h"
#include "../src/dockmodel.h"
#include "../src/dockmodelops.h"
#include "../src/dockshell.h"
#include "../src/panitentapp.h"
#include "../src/panitentwindow.h"
#include "../src/windowlayoutmanager.h"
#include "../src/win32/window.h"

typedef struct DockRuntimeFixture
{
    PanitentApp* pApp;
    Window* pFrame;
    HWND hWndFrame;
    DockHostWindow* pDockHostWindow;
    HWND hWndDockHost;
    PanitentWindow panitentWindow;
} DockRuntimeFixture;

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

int main(void)
{
    HRESULT hrOle = OleInitialize(NULL);
    BOOL bOleInitialized = SUCCEEDED(hrOle);
    int failed = 0;

    failed |= test_runtime_tool_model_docking_moves_panel_between_zones();
    failed |= test_runtime_window_layout_apply_invalid_bundle_rolls_back();
    failed |= test_runtime_window_layout_apply_and_reset_preserves_workspace();

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

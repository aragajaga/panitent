#include "precomp.h"

#include "dockhostmodelapply.h"

#include "dockhostpreserve.h"
#include "dockhostrestore.h"
#include "dockhosttree.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockmodelmatch.h"
#include "dockmodelops.h"
#include "dockmodelvalidate.h"
#include "dockviewcatalog.h"
#include "floatingdocumenthost.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"

static DockModelNode* DockHostModelApply_CreatePanelModel(HWND hWnd)
{
    PanitentDockViewId nViewId = DockHostPreserve_GetViewIdForHwnd(hWnd);
    PCWSTR pszName = PanitentDockViewCatalog_GetCanonicalName(nViewId);
    WCHAR szCaption[MAX_PATH] = L"";
    GetWindowTextW(hWnd, szCaption, ARRAYSIZE(szCaption));

    if (nViewId == PNT_DOCK_VIEW_NONE || !pszName || !pszName[0])
    {
        return NULL;
    }

    DockModelNode* pPanelNode = (DockModelNode*)calloc(1, sizeof(DockModelNode));
    if (!pPanelNode)
    {
        return NULL;
    }

    pPanelNode->uViewId = (uint32_t)nViewId;
    pPanelNode->nRole = DOCK_ROLE_PANEL;
    pPanelNode->nPaneKind = DOCK_PANE_TOOL;
    pPanelNode->dwStyle = DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL;
    pPanelNode->iGripPos = 64;
    pPanelNode->fGripPos = -1.0f;
    pPanelNode->bShowCaption = TRUE;
    wcscpy_s(pPanelNode->szName, ARRAYSIZE(pPanelNode->szName), pszName);
    wcscpy_s(pPanelNode->szCaption, ARRAYSIZE(pPanelNode->szCaption), szCaption[0] ? szCaption : pszName);
    return pPanelNode;
}

static DockModelNode* DockHostModelApply_CreateWorkspaceModel(HWND hWnd)
{
    if (!DockHostPreserve_IsWorkspaceHwnd(hWnd))
    {
        return NULL;
    }

    DockModelNode* pWorkspaceNode = (DockModelNode*)calloc(1, sizeof(DockModelNode));
    if (!pWorkspaceNode)
    {
        return NULL;
    }

    pWorkspaceNode->uViewId = (uint32_t)PNT_DOCK_VIEW_WORKSPACE;
    pWorkspaceNode->nRole = DOCK_ROLE_WORKSPACE;
    pWorkspaceNode->nPaneKind = DOCK_PANE_DOCUMENT;
    pWorkspaceNode->dwStyle = DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL;
    pWorkspaceNode->iGripPos = 64;
    pWorkspaceNode->fGripPos = -1.0f;
    pWorkspaceNode->bShowCaption = FALSE;
    wcscpy_s(pWorkspaceNode->szName, ARRAYSIZE(pWorkspaceNode->szName), L"WorkspaceContainer");
    return pWorkspaceNode;
}

static BOOL DockHostModelApply_ApplyModel(DockHostWindow* pDockHostWindow, DockModelNode* pModelRoot, DockHostModelApplyContext* pContext)
{
    if (!pDockHostWindow || !pModelRoot || !pContext)
    {
        return FALSE;
    }

    TreeNode* pRootNode = DockModelBuildTree(pModelRoot);
    if (!pRootNode || !pRootNode->data)
    {
        DockModelBuildDestroyTree(pRootNode);
        return FALSE;
    }

    RECT rcDockHost = { 0 };
    Window_GetClientRect((Window*)pDockHostWindow, &rcDockHost);
    ((DockData*)pRootNode->data)->rc = rcDockHost;

    HWND hPreserve[16] = { 0 };
    int nPreserve = 0;
    DockHostPreserve_FillPreserveArray(pContext, hPreserve, ARRAYSIZE(hPreserve), &nPreserve);
    DockHostWindow_ClearLayout(pDockHostWindow, hPreserve, nPreserve);

    BOOL bHasWorkspace = FALSE;
    if (!PanitentDockHostRestoreAttachKnownViewsEx(
        pContext->pPanitentApp,
        pDockHostWindow,
        pRootNode,
        DockHostPreserve_ResolveView,
        pContext,
        NULL,
        NULL,
        &bHasWorkspace))
    {
        DockHostWindow_DestroyNodeTree(pRootNode, hPreserve, nPreserve);
        return FALSE;
    }

    DockHostWindow_SetRoot(pDockHostWindow, pRootNode);
    DockHostWindow_Rearrange(pDockHostWindow);
    return bHasWorkspace;
}

BOOL DockHostModelApply_DockToolWindow(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !pTargetHit || pTargetHit->nDockSide == DKS_NONE)
    {
        return FALSE;
    }

    DockModelNode* pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    DockModelNode* pTargetModel = DockModelOps_CloneTree(pRollbackModel);
    DockModelNode* pPanelNode = DockHostModelApply_CreatePanelModel(hWnd);
    if (!pRollbackModel || !pTargetModel || !pPanelNode)
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        DockModel_Destroy(pPanelNode);
        return FALSE;
    }

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostPreserve_CollectViewsRecursive(
        &context,
        DockHostWindow_GetRoot(pDockHostWindow),
        pRollbackModel,
        DockHostWindow_GetRoot(pDockHostWindow));
    DockHostPreserve_AddView(
        &context,
        DockHostPreserve_GetViewIdForHwnd(hWnd),
        0,
        hWnd,
        FALSE);

    BOOL bMutated = FALSE;
    if (pTargetHit->bLocalTarget && pTargetHit->hWndAnchor && IsWindow(pTargetHit->hWndAnchor))
    {
        TreeNode* pAnchorLiveNode = DockNode_FindByHWND(DockHostWindow_GetRoot(pDockHostWindow), pTargetHit->hWndAnchor);
        DockData* pAnchorLiveData = pAnchorLiveNode ? (DockData*)pAnchorLiveNode->data : NULL;
        if (pAnchorLiveData)
        {
            DockModelNode* pAnchorModelNode = DockModelMatch_FindNodeForLiveData(
                DockHostWindow_GetRoot(pDockHostWindow),
                pTargetModel,
                pAnchorLiveData);
            if (pTargetHit->nDockSide == DKS_CENTER)
            {
                int nZoneSide = DockHostWindow_GetPanelDockSide(pDockHostWindow, pTargetHit->hWndAnchor);
                bMutated = DockModelOps_AppendPanelToZone(pTargetModel, nZoneSide, pPanelNode);
            }
            else if (pAnchorModelNode) {
                bMutated = DockModelOps_DockPanelAroundNode(pTargetModel, pAnchorModelNode->uNodeId, pTargetHit->nDockSide, pPanelNode);
            }
        }
    }
    else {
        bMutated = DockModelOps_DockPanelAtRootSide(pTargetModel, pTargetHit->nDockSide, pPanelNode);
    }

    if (!bMutated || !DockModelValidateAndRepairMainLayout(&pTargetModel, NULL))
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    BOOL bApplied = DockHostModelApply_ApplyModel(pDockHostWindow, pTargetModel, &context);
    if (!bApplied)
    {
        DockHostModelApply_ApplyModel(pDockHostWindow, pRollbackModel, &context);
    }

    DockModel_Destroy(pRollbackModel);
    DockModel_Destroy(pTargetModel);
    if (bApplied)
    {
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    return bApplied;
}

BOOL DockHostModelApply_RemoveDockedWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    TreeNode* pLiveNode = DockNode_FindByHWND(DockHostWindow_GetRoot(pDockHostWindow), hWnd);
    DockData* pLiveData = pLiveNode ? (DockData*)pLiveNode->data : NULL;
    if (!pLiveData)
    {
        return FALSE;
    }

    if (pLiveData->nPaneKind == DOCK_PANE_TOOL)
    {
        return DockHostModelApply_RemoveToolWindow(pDockHostWindow, hWnd, bKeepWindowAlive);
    }
    if (pLiveData->nPaneKind == DOCK_PANE_DOCUMENT)
    {
        return DockHostModelApply_RemoveDocumentWindow(pDockHostWindow, hWnd, bKeepWindowAlive);
    }

    return FALSE;
}

BOOL DockHostModelApply_RemoveToolWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    TreeNode* pLiveNode = DockNode_FindByHWND(DockHostWindow_GetRoot(pDockHostWindow), hWnd);
    DockData* pLiveData = pLiveNode ? (DockData*)pLiveNode->data : NULL;
    if (!pLiveData || pLiveData->nPaneKind != DOCK_PANE_TOOL)
    {
        return FALSE;
    }

    DockModelNode* pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    DockModelNode* pTargetModel = DockModelOps_CloneTree(pRollbackModel);
    if (!pRollbackModel || !pTargetModel)
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostPreserve_CollectViewsRecursiveEx(
        &context,
        DockHostWindow_GetRoot(pDockHostWindow),
        pRollbackModel,
        DockHostWindow_GetRoot(pDockHostWindow),
        hWnd);

    DockModelNode* pLiveModelNode = DockModelMatch_FindNodeForLiveData(
        DockHostWindow_GetRoot(pDockHostWindow),
        pTargetModel,
        pLiveData);
    if (!pLiveModelNode ||
        !DockModelOps_RemoveNodeById(&pTargetModel, pLiveModelNode->uNodeId) ||
        !DockModelValidateAndRepairMainLayout(&pTargetModel, NULL))
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    BOOL bApplied = DockHostModelApply_ApplyModel(pDockHostWindow, pTargetModel, &context);
    if (!bApplied)
    {
        DockHostModelApply_ApplyModel(pDockHostWindow, pRollbackModel, &context);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    else if (!bKeepWindowAlive)
    {
        DestroyWindow(hWnd);
    }

    DockModel_Destroy(pRollbackModel);
    DockModel_Destroy(pTargetModel);
    return bApplied;
}

BOOL DockHostModelApply_DockDocumentWindow(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
    UNREFERENCED_PARAMETER(iDockSize);

    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !DockHostPreserve_IsWorkspaceHwnd(hWnd) ||
        !pTargetHit || !pTargetHit->bLocalTarget || pTargetHit->nDockSide == DKS_NONE ||
        pTargetHit->nDockSide == DKS_CENTER || !pTargetHit->hWndAnchor || !IsWindow(pTargetHit->hWndAnchor))
    {
        return FALSE;
    }

    TreeNode* pLiveRoot = DockHostWindow_GetRoot(pDockHostWindow);
    TreeNode* pAnchorLiveNode = DockNode_FindByHWND(pLiveRoot, pTargetHit->hWndAnchor);
    DockData* pAnchorLiveData = pAnchorLiveNode ? (DockData*)pAnchorLiveNode->data : NULL;
    if (!pAnchorLiveData || pAnchorLiveData->nRole != DOCK_ROLE_WORKSPACE)
    {
        return FALSE;
    }

    DockModelNode* pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    DockModelNode* pTargetModel = DockModelOps_CloneTree(pRollbackModel);
    DockModelNode* pWorkspaceNode = DockHostModelApply_CreateWorkspaceModel(hWnd);
    if (!pRollbackModel || !pTargetModel || !pWorkspaceNode)
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        DockModel_Destroy(pWorkspaceNode);
        return FALSE;
    }

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostPreserve_CollectViewsRecursive(&context, pLiveRoot, pRollbackModel, pLiveRoot);

    DockModelNode* pAnchorModelNode = DockModelMatch_FindNodeForLiveData(
        pLiveRoot,
        pTargetModel,
        pAnchorLiveData);
    if (!pAnchorModelNode ||
        !DockModelOps_DockWorkspaceAroundNode(pTargetModel, pAnchorModelNode->uNodeId, pTargetHit->nDockSide, pWorkspaceNode))
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    DockHostPreserve_AddView(
        &context,
        PNT_DOCK_VIEW_WORKSPACE,
        pWorkspaceNode->uNodeId,
        hWnd,
        FALSE);

    if (!DockModelValidateAndRepairMainLayout(&pTargetModel, NULL))
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    BOOL bApplied = DockHostModelApply_ApplyModel(pDockHostWindow, pTargetModel, &context);
    if (!bApplied)
    {
        DockHostModelApply_ApplyModel(pDockHostWindow, pRollbackModel, &context);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    else {
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }

    DockModel_Destroy(pRollbackModel);
    DockModel_Destroy(pTargetModel);
    return bApplied;
}

BOOL DockHostModelApply_RemoveDocumentWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !DockHostPreserve_IsWorkspaceHwnd(hWnd))
    {
        return FALSE;
    }

    TreeNode* pLiveRoot = DockHostWindow_GetRoot(pDockHostWindow);
    TreeNode* pLiveNode = DockNode_FindByHWND(pLiveRoot, hWnd);
    DockData* pLiveData = pLiveNode ? (DockData*)pLiveNode->data : NULL;
    if (!pLiveData || pLiveData->nRole != DOCK_ROLE_WORKSPACE)
    {
        return FALSE;
    }

    HWND hWndMainWorkspace = NULL;
    if (PanitentApp_Instance() && PanitentApp_Instance()->m_pWorkspaceContainer)
    {
        hWndMainWorkspace = Window_GetHWND((Window*)PanitentApp_Instance()->m_pWorkspaceContainer);
    }
    if (hWnd == hWndMainWorkspace)
    {
        return FALSE;
    }

    DockModelNode* pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    DockModelNode* pTargetModel = DockModelOps_CloneTree(pRollbackModel);
    if (!pRollbackModel || !pTargetModel)
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostPreserve_CollectViewsRecursiveEx(
        &context,
        pLiveRoot,
        pRollbackModel,
        pLiveRoot,
        hWnd);

    DockModelNode* pLiveModelNode = DockModelMatch_FindNodeForLiveData(pLiveRoot, pTargetModel, pLiveData);
    if (!pLiveModelNode ||
        !DockModelOps_RemoveNodeById(&pTargetModel, pLiveModelNode->uNodeId) ||
        !DockModelValidateAndRepairMainLayout(&pTargetModel, NULL))
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    BOOL bApplied = DockHostModelApply_ApplyModel(pDockHostWindow, pTargetModel, &context);
    if (!bApplied)
    {
        DockHostModelApply_ApplyModel(pDockHostWindow, pRollbackModel, &context);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    else if (!bKeepWindowAlive)
    {
        DestroyWindow(hWnd);
    }

    DockModel_Destroy(pRollbackModel);
    DockModel_Destroy(pTargetModel);
    return bApplied;
}

BOOL DockHostModelApply_UndockDocumentWindowToFloating(
    DockHostWindow* pDockHostWindow,
    HWND hWnd,
    const RECT* pFloatingWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut)
{
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = NULL;
    }

    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !DockHostPreserve_IsWorkspaceHwnd(hWnd))
    {
        return FALSE;
    }

    TreeNode* pLiveRoot = DockHostWindow_GetRoot(pDockHostWindow);
    TreeNode* pLiveNode = DockNode_FindByHWND(pLiveRoot, hWnd);
    DockData* pLiveData = pLiveNode ? (DockData*)pLiveNode->data : NULL;
    if (!pLiveData || pLiveData->nRole != DOCK_ROLE_WORKSPACE)
    {
        return FALSE;
    }

    HWND hWndMainWorkspace = NULL;
    if (PanitentApp_Instance() && PanitentApp_Instance()->m_pWorkspaceContainer)
    {
        hWndMainWorkspace = Window_GetHWND((Window*)PanitentApp_Instance()->m_pWorkspaceContainer);
    }
    if (hWnd == hWndMainWorkspace)
    {
        return FALSE;
    }

    DockModelNode* pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    DockModelNode* pTargetModel = DockModelOps_CloneTree(pRollbackModel);
    if (!pRollbackModel || !pTargetModel)
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostPreserve_CollectViewsRecursiveEx(
        &context,
        pLiveRoot,
        pRollbackModel,
        pLiveRoot,
        hWnd);

    DockModelNode* pLiveModelNode = DockModelMatch_FindNodeForLiveData(pLiveRoot, pTargetModel, pLiveData);
    if (!pLiveModelNode ||
        !DockModelOps_RemoveNodeById(&pTargetModel, pLiveModelNode->uNodeId) ||
        !DockModelValidateAndRepairMainLayout(&pTargetModel, NULL))
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    if (!DockHostModelApply_ApplyModel(pDockHostWindow, pTargetModel, &context))
    {
        DockHostModelApply_ApplyModel(pDockHostWindow, pRollbackModel, &context);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    HWND hWndFloating = NULL;
    if (!FloatingDocumentHost_CreatePinnedWindow(
        pDockHostWindow,
        hWnd,
        pFloatingWindowRect,
        bStartMove,
        ptMoveScreen,
        &hWndFloating))
    {
        DockHostModelApply_ApplyModel(pDockHostWindow, pRollbackModel, &context);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    DockModel_Destroy(pRollbackModel);
    DockModel_Destroy(pTargetModel);
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

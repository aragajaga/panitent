#include "precomp.h"

#include "dockhostmodelapply.h"

#include "dockhostapplycore.h"
#include "dockhostpreserve.h"
#include "dockhostapplytxn.h"
#include "dockhostrestore.h"
#include "dockhosttree.h"
#include "dockmodel.h"
#include "dockmodelmatch.h"
#include "dockmodelops.h"
#include "dockmodelvalidate.h"
#include "floatingdocumenthost.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"

BOOL DockHostModelApply_DockToolWindow(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !pTargetHit || pTargetHit->nDockSide == DKS_NONE)
    {
        return FALSE;
    }

    DockHostApplyTxn txn = { 0 };
    DockModelNode* pPanelNode = DockHostApplyCore_CreatePanelModel(hWnd);
    if (!DockHostApplyTxn_Begin(&txn, pDockHostWindow, NULL) || !pPanelNode)
    {
        DockModel_Destroy(pPanelNode);
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    DockHostPreserve_AddView(
        &txn.context,
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
                txn.pTargetModel,
                pAnchorLiveData);
            if (pTargetHit->nDockSide == DKS_CENTER)
            {
                int nZoneSide = DockHostWindow_GetPanelDockSide(pDockHostWindow, pTargetHit->hWndAnchor);
                bMutated = DockModelOps_AppendPanelToZone(txn.pTargetModel, nZoneSide, pPanelNode);
            }
            else if (pAnchorModelNode) {
                bMutated = DockModelOps_DockPanelAroundNode(txn.pTargetModel, pAnchorModelNode->uNodeId, pTargetHit->nDockSide, pPanelNode);
            }
        }
    }
    else {
        bMutated = DockModelOps_DockPanelAtRootSide(txn.pTargetModel, pTargetHit->nDockSide, pPanelNode);
    }

    if (!bMutated || !DockModelValidateAndRepairMainLayout(&txn.pTargetModel, NULL))
    {
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    BOOL bApplied = DockHostApplyCore_ApplyModel(pDockHostWindow, txn.pTargetModel, &txn.context);
    if (!bApplied)
    {
        DockHostApplyTxn_Rollback(pDockHostWindow, &txn);
    }

    DockHostApplyTxn_End(&txn);
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

    DockHostApplyTxn txn = { 0 };
    if (!DockHostApplyTxn_Begin(&txn, pDockHostWindow, hWnd))
    {
        return FALSE;
    }

    DockModelNode* pLiveModelNode = DockModelMatch_FindNodeForLiveData(
        DockHostWindow_GetRoot(pDockHostWindow),
        txn.pTargetModel,
        pLiveData);
    if (!pLiveModelNode ||
        !DockModelOps_RemoveNodeById(&txn.pTargetModel, pLiveModelNode->uNodeId) ||
        !DockModelValidateAndRepairMainLayout(&txn.pTargetModel, NULL))
    {
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    BOOL bApplied = DockHostApplyCore_ApplyModel(pDockHostWindow, txn.pTargetModel, &txn.context);
    if (!bApplied)
    {
        DockHostApplyTxn_Rollback(pDockHostWindow, &txn);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    else if (!bKeepWindowAlive)
    {
        DestroyWindow(hWnd);
    }

    DockHostApplyTxn_End(&txn);
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

    DockHostApplyTxn txn = { 0 };
    DockModelNode* pWorkspaceNode = DockHostApplyCore_CreateWorkspaceModel(hWnd);
    if (!DockHostApplyTxn_Begin(&txn, pDockHostWindow, NULL) || !pWorkspaceNode)
    {
        DockModel_Destroy(pWorkspaceNode);
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    DockModelNode* pAnchorModelNode = DockModelMatch_FindNodeForLiveData(
        pLiveRoot,
        txn.pTargetModel,
        pAnchorLiveData);
    if (!pAnchorModelNode ||
        !DockModelOps_DockWorkspaceAroundNode(txn.pTargetModel, pAnchorModelNode->uNodeId, pTargetHit->nDockSide, pWorkspaceNode))
    {
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    DockHostPreserve_AddView(
        &txn.context,
        PNT_DOCK_VIEW_WORKSPACE,
        pWorkspaceNode->uNodeId,
        hWnd,
        FALSE);

    if (!DockModelValidateAndRepairMainLayout(&txn.pTargetModel, NULL))
    {
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    BOOL bApplied = DockHostApplyCore_ApplyModel(pDockHostWindow, txn.pTargetModel, &txn.context);
    if (!bApplied)
    {
        DockHostApplyTxn_Rollback(pDockHostWindow, &txn);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    else {
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }

    DockHostApplyTxn_End(&txn);
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

    DockHostApplyTxn txn = { 0 };
    if (!DockHostApplyTxn_Begin(&txn, pDockHostWindow, hWnd))
    {
        return FALSE;
    }

    DockModelNode* pLiveModelNode = DockModelMatch_FindNodeForLiveData(pLiveRoot, txn.pTargetModel, pLiveData);
    if (!pLiveModelNode ||
        !DockModelOps_RemoveNodeById(&txn.pTargetModel, pLiveModelNode->uNodeId) ||
        !DockModelValidateAndRepairMainLayout(&txn.pTargetModel, NULL))
    {
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    BOOL bApplied = DockHostApplyCore_ApplyModel(pDockHostWindow, txn.pTargetModel, &txn.context);
    if (!bApplied)
    {
        DockHostApplyTxn_Rollback(pDockHostWindow, &txn);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }
    else if (!bKeepWindowAlive)
    {
        DestroyWindow(hWnd);
    }

    DockHostApplyTxn_End(&txn);
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

    DockHostApplyTxn txn = { 0 };
    if (!DockHostApplyTxn_Begin(&txn, pDockHostWindow, hWnd))
    {
        return FALSE;
    }

    DockModelNode* pLiveModelNode = DockModelMatch_FindNodeForLiveData(pLiveRoot, txn.pTargetModel, pLiveData);
    if (!pLiveModelNode ||
        !DockModelOps_RemoveNodeById(&txn.pTargetModel, pLiveModelNode->uNodeId) ||
        !DockModelValidateAndRepairMainLayout(&txn.pTargetModel, NULL))
    {
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    if (!DockHostApplyCore_ApplyModel(pDockHostWindow, txn.pTargetModel, &txn.context))
    {
        DockHostApplyTxn_Rollback(pDockHostWindow, &txn);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        DockHostApplyTxn_End(&txn);
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
        DockHostApplyTxn_Rollback(pDockHostWindow, &txn);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        DockHostApplyTxn_End(&txn);
        return FALSE;
    }

    DockHostApplyTxn_End(&txn);
    if (phWndFloatingOut)
    {
        *phWndFloatingOut = hWndFloating;
    }
    return TRUE;
}

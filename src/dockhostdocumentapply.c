#include "precomp.h"

#include "dockhostmodelapply.h"

#include "dockhostapplycore.h"
#include "dockhostapplytxn.h"
#include "dockhostpreserve.h"
#include "dockhosttree.h"
#include "dockmodel.h"
#include "dockmodelmatch.h"
#include "dockmodelops.h"
#include "dockmodelvalidate.h"
#include "floatingdocumenthost.h"
#include "panitentapp.h"
#include "win32/window.h"

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

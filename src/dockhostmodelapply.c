#include "precomp.h"

#include "dockhostmodelapply.h"

#include "dockhostapplycore.h"
#include "dockhostpreserve.h"
#include "dockhostapplytxn.h"
#include "dockhosttree.h"
#include "dockmodelmatch.h"
#include "dockmodelops.h"
#include "dockmodelvalidate.h"
#include "win32/window.h"

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

#include "precomp.h"

#include "dockhostmutate.h"

#include "dockhostbinding.h"
#include "dockhostdocumentapply.h"
#include "dockhosttoolapply.h"
#include "win32/window.h"

static BOOL DockHostMutate_NodeIsStructural(TreeNode* pNode)
{
    if (!pNode || !pNode->data)
    {
        return FALSE;
    }

    DockData* pDockData = (DockData*)pNode->data;
    return DockNodeRole_IsStructural(pDockData->nRole, pDockData->lpszName);
}

void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
    DockHostMutate_DestroyInclusive(pDockHostWindow, pTargetNode);
}

void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
    DockHostMutate_Undock(pDockHostWindow, pTargetNode);
}

BOOL DockHostWindow_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize)
{
    return DockHostMutate_DockHWND(pDockHostWindow, hWnd, nDockSide, iDockSize);
}

BOOL DockHostWindow_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
    return DockHostMutate_DockHWNDToTarget(pDockHostWindow, hWnd, pTargetHit, iDockSize);
}

BOOL DockHostWindow_DestroyDockedHWND(DockHostWindow* pDockHostWindow, HWND hWnd)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
    if (!pRoot)
    {
        return FALSE;
    }

    TreeNode* pNode = DockNode_FindByHWND(pRoot, hWnd);
    if (!pNode)
    {
        return FALSE;
    }

    DockHostWindow_DestroyInclusive(pDockHostWindow, pNode);
    return TRUE;
}

BOOL DockHostMutate_RemoveDockedWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive)
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

void DockHostMutate_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
    if (!pDockHostWindow || !pTargetNode || !pTargetNode->data)
    {
        return;
    }

    if (pDockHostWindow->pCaptionHotNode == pTargetNode || pDockHostWindow->pCaptionPressedNode == pTargetNode)
    {
        pDockHostWindow->pCaptionHotNode = NULL;
        pDockHostWindow->pCaptionPressedNode = NULL;
        pDockHostWindow->nCaptionHotButton = 0;
        pDockHostWindow->nCaptionPressedButton = 0;
    }

    DockData* pDockData = (DockData*)pTargetNode->data;
    if (pDockData->hWnd && IsWindow(pDockData->hWnd))
    {
        if (DockHostMutate_RemoveDockedWindow(pDockHostWindow, pDockData->hWnd, FALSE))
        {
            return;
        }
        DestroyWindow(pDockData->hWnd);
    }

    DockHostMutate_Undock(pDockHostWindow, pTargetNode);
}

void DockHostMutate_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
    TreeNode* pRoot = pDockHostWindow ? DockHostWindow_GetRoot(pDockHostWindow) : NULL;
    if (!pRoot || !pTargetNode)
    {
        return;
    }

    TreeNode* pParentOfTarget = DockNode_FindParent(pRoot, pTargetNode);
    if (!pParentOfTarget)
    {
        return;
    }

    if (DockHostMutate_NodeIsStructural(pParentOfTarget))
    {
        if (pTargetNode == pParentOfTarget->node1)
        {
            pParentOfTarget->node1 = pParentOfTarget->node2;
            pParentOfTarget->node2 = NULL;
        }
        else if (pTargetNode == pParentOfTarget->node2)
        {
            pParentOfTarget->node2 = NULL;
        }

        DockHostWindow_Rearrange(pDockHostWindow);
        return;
    }

    TreeNode* pNodeSibling = NULL;
    if (pTargetNode == pParentOfTarget->node1)
    {
        pParentOfTarget->node1 = NULL;
        pNodeSibling = pParentOfTarget->node2;
    }
    else if (pTargetNode == pParentOfTarget->node2)
    {
        pParentOfTarget->node2 = NULL;
        pNodeSibling = pParentOfTarget->node1;
    }

    TreeNode* pGrandparentOfTarget = DockNode_FindParent(pRoot, pParentOfTarget);
    if (pGrandparentOfTarget)
    {
        if (pGrandparentOfTarget->node1 == pParentOfTarget)
        {
            pGrandparentOfTarget->node1 = pNodeSibling;
            free(pParentOfTarget);
        }
        else if (pGrandparentOfTarget->node2 == pParentOfTarget)
        {
            pGrandparentOfTarget->node2 = pNodeSibling;
            free(pParentOfTarget);
        }
    }
    else {
        if (pTargetNode == pRoot->node1)
        {
            pRoot->node1 = pNodeSibling;
        }
        else if (pTargetNode == pRoot->node2)
        {
            pRoot->node2 = pNodeSibling;
        }
    }

    DockHostWindow_Rearrange(pDockHostWindow);
}

BOOL DockHostMutate_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    if (nPaneKind == DOCK_PANE_TOOL)
    {
        DockTargetHit targetHit = { 0 };
        targetHit.nDockSide = nDockSide;
        targetHit.bLocalTarget = FALSE;
        return DockHostModelApply_DockToolWindow(pDockHostWindow, hWnd, &targetHit, iDockSize);
    }
    if (nPaneKind == DOCK_PANE_DOCUMENT)
    {
        return FALSE;
    }
    return FALSE;
}

BOOL DockHostMutate_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !pTargetHit || pTargetHit->nDockSide == DKS_NONE)
    {
        return FALSE;
    }

    DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    if (nPaneKind == DOCK_PANE_TOOL)
    {
        return DockHostModelApply_DockToolWindow(pDockHostWindow, hWnd, pTargetHit, iDockSize);
    }
    if (nPaneKind == DOCK_PANE_DOCUMENT)
    {
        return DockHostModelApply_DockDocumentWindow(pDockHostWindow, hWnd, pTargetHit, iDockSize);
    }
    return FALSE;
}

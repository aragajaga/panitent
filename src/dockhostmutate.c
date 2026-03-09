#include "precomp.h"

#include "dockhostmutate.h"

#include "dockhostmodelapply.h"
#include "dockgroup.h"
#include "docklayout.h"
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

static void DockHostMutate_UpdateZoneSplitGrip(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, int nDockSide, int iDockSize)
{
    if (!pDockHostWindow || !pZoneNode)
    {
        return;
    }

    TreeNode* pParent = DockNode_FindParent(DockHostWindow_GetRoot(pDockHostWindow), pZoneNode);
    if (!pParent || !pParent->data)
    {
        return;
    }

    DockData* pDockDataParent = (DockData*)pParent->data;
    pDockDataParent->iGripPos = DockLayout_GetZoneSplitGrip(nDockSide, iDockSize);
}

static BOOL DockHostMutate_DockIntoZone(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, HWND hWnd, int nDockSide, int iDockSize)
{
    if (!pDockHostWindow || !pZoneNode || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    DockData* pZoneData = pZoneNode->data ? (DockData*)pZoneNode->data : NULL;
    DockHostMutate_UpdateZoneSplitGrip(pDockHostWindow, pZoneNode, nDockSide, iDockSize);

    TreeNode* pLeaf = DockNode_Create(DockLayout_GetZoneStackGrip(nDockSide, iDockSize), DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, TRUE);
    if (!pLeaf || !pLeaf->data)
    {
        return FALSE;
    }

    DockData* pDockDataLeaf = (DockData*)pLeaf->data;
    DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    DockData_PinHWND(pDockHostWindow, pDockDataLeaf, hWnd);
    pDockDataLeaf->nRole = DOCK_ROLE_PANEL;
    pDockDataLeaf->nPaneKind = nPaneKind;

    if (!pZoneNode->node1)
    {
        pZoneNode->node1 = pLeaf;
        if (pZoneData)
        {
            pZoneData->hWndActiveTab = hWnd;
        }
        return TRUE;
    }

    TreeNode* pSplit = DockNode_Create(DockLayout_GetZoneStackGrip(nDockSide, iDockSize), DockLayout_GetZoneStackStyle(nDockSide), FALSE);
    if (!pSplit || !pSplit->data)
    {
        free(pLeaf->data);
        free(pLeaf);
        return FALSE;
    }

    DockData* pDockDataSplit = (DockData*)pSplit->data;
    wcscpy_s(pDockDataSplit->lpszName, MAX_PATH, L"DockShell.ZoneStack");
    pDockDataSplit->nRole = DOCK_ROLE_ZONE_STACK_SPLIT;

    pSplit->node1 = pZoneNode->node1;
    pSplit->node2 = pLeaf;
    pZoneNode->node1 = pSplit;
    if (pZoneData)
    {
        pZoneData->hWndActiveTab = hWnd;
    }

    return TRUE;
}

static BOOL DockHostMutate_DockAroundPanel(DockHostWindow* pDockHostWindow, TreeNode* pAnchorNode, HWND hWnd, int nDockSide, int iDockSize)
{
    if (!pDockHostWindow || !pAnchorNode || !pAnchorNode->data || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    DockData* pAnchorData = (DockData*)pAnchorNode->data;
    if (!pAnchorData->hWnd || !IsWindow(pAnchorData->hWnd))
    {
        return FALSE;
    }

    DWORD dwSplitStyle = DGP_ABSOLUTE;
    switch (nDockSide)
    {
    case DKS_LEFT:
    case DKS_RIGHT:
        dwSplitStyle |= DGD_HORIZONTAL;
        break;
    case DKS_TOP:
    case DKS_BOTTOM:
        dwSplitStyle |= DGD_VERTICAL;
        break;
    default:
        return FALSE;
    }

    if (nDockSide == DKS_RIGHT || nDockSide == DKS_BOTTOM)
    {
        dwSplitStyle |= DGA_END;
    }
    else {
        dwSplitStyle |= DGA_START;
    }

    if (iDockSize <= 0)
    {
        iDockSize = 280;
    }

    TreeNode* pLeaf = DockNode_Create(DockLayout_GetZoneStackGrip(nDockSide, iDockSize), DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, TRUE);
    TreeNode* pSplit = DockNode_Create(DockLayout_GetZoneSplitGrip(nDockSide, iDockSize), dwSplitStyle, FALSE);
    if (!pLeaf || !pLeaf->data || !pSplit || !pSplit->data)
    {
        if (pLeaf)
        {
            free(pLeaf->data);
            free(pLeaf);
        }
        if (pSplit)
        {
            free(pSplit->data);
            free(pSplit);
        }
        return FALSE;
    }

    DockData* pLeafData = (DockData*)pLeaf->data;
    DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    DockData_PinHWND(pDockHostWindow, pLeafData, hWnd);
    pLeafData->nRole = DOCK_ROLE_PANEL;
    pLeafData->nPaneKind = nPaneKind;

    DockData* pSplitData = (DockData*)pSplit->data;
    wcscpy_s(pSplitData->lpszName, MAX_PATH, L"DockShell.PanelSplit");
    pSplitData->rc = pAnchorData->rc;
    pSplitData->nRole = DOCK_ROLE_PANEL_SPLIT;

    if (nDockSide == DKS_LEFT || nDockSide == DKS_TOP)
    {
        pSplit->node1 = pLeaf;
        pSplit->node2 = pAnchorNode;
    }
    else {
        pSplit->node1 = pAnchorNode;
        pSplit->node2 = pLeaf;
    }

    TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
    TreeNode* pParent = DockNode_FindParent(pRoot, pAnchorNode);
    if (pParent)
    {
        if (pParent->node1 == pAnchorNode)
        {
            pParent->node1 = pSplit;
        }
        else if (pParent->node2 == pAnchorNode)
        {
            pParent->node2 = pSplit;
        }
    }
    else if (pRoot == pAnchorNode) {
        DockHostWindow_SetRoot(pDockHostWindow, pSplit);
    }
    else {
        free(pLeaf->data);
        free(pLeaf);
        free(pSplit->data);
        free(pSplit);
        return FALSE;
    }

    DockHostWindow_Rearrange(pDockHostWindow);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return TRUE;
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
        if (pDockData->nPaneKind == DOCK_PANE_TOOL &&
            DockHostModelApply_RemoveToolWindow(pDockHostWindow, pDockData->hWnd, FALSE))
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

    if (DockHostWindow_DeterminePaneKindForHWND(hWnd) == DOCK_PANE_TOOL)
    {
        DockTargetHit targetHit = { 0 };
        targetHit.nDockSide = nDockSide;
        targetHit.bLocalTarget = FALSE;
        if (DockHostModelApply_DockToolWindow(pDockHostWindow, hWnd, &targetHit, iDockSize))
        {
            return TRUE;
        }
    }

    TreeNode* pOldRoot = DockHostWindow_GetRoot(pDockHostWindow);
    if (!pOldRoot)
    {
        return FALSE;
    }

    TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, nDockSide);
    if (pZoneNode)
    {
        if (!DockHostMutate_DockIntoZone(pDockHostWindow, pZoneNode, hWnd, nDockSide, iDockSize))
        {
            return FALSE;
        }

        DockHostWindow_Rearrange(pDockHostWindow);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        return TRUE;
    }

    DWORD dwSplitStyle = 0;
    switch (nDockSide)
    {
    case DKS_LEFT:
    case DKS_RIGHT:
        dwSplitStyle = DGP_ABSOLUTE | DGD_HORIZONTAL;
        break;
    case DKS_TOP:
    case DKS_BOTTOM:
        dwSplitStyle = DGP_ABSOLUTE | DGD_VERTICAL;
        break;
    default:
        return FALSE;
    }

    if (nDockSide == DKS_RIGHT || nDockSide == DKS_BOTTOM)
    {
        dwSplitStyle |= DGA_END;
    }
    else {
        dwSplitStyle |= DGA_START;
    }

    if (iDockSize <= 0)
    {
        iDockSize = 280;
    }

    TreeNode* pLeaf = DockNode_Create(iDockSize, DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, TRUE);
    TreeNode* pSplit = DockNode_Create(iDockSize, dwSplitStyle, FALSE);
    if (!pLeaf || !pLeaf->data || !pSplit || !pSplit->data)
    {
        if (pLeaf)
        {
            free(pLeaf->data);
            free(pLeaf);
        }
        if (pSplit)
        {
            free(pSplit->data);
            free(pSplit);
        }
        return FALSE;
    }

    DockData* pLeafData = (DockData*)pLeaf->data;
    DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    DockData_PinHWND(pDockHostWindow, pLeafData, hWnd);
    pLeafData->nRole = DOCK_ROLE_PANEL;
    pLeafData->nPaneKind = nPaneKind;

    if (nDockSide == DKS_LEFT || nDockSide == DKS_TOP)
    {
        pSplit->node1 = pLeaf;
        pSplit->node2 = pOldRoot;
    }
    else {
        pSplit->node1 = pOldRoot;
        pSplit->node2 = pLeaf;
    }

    RECT rcClient = { 0 };
    GetClientRect(Window_GetHWND((Window*)pDockHostWindow), &rcClient);
    ((DockData*)pSplit->data)->rc = rcClient;
    ((DockData*)pSplit->data)->nRole = DOCK_ROLE_SHELL_SPLIT;

    DockHostWindow_SetRoot(pDockHostWindow, pSplit);
    DockHostWindow_Rearrange(pDockHostWindow);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return TRUE;
}

BOOL DockHostMutate_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !pTargetHit || pTargetHit->nDockSide == DKS_NONE)
    {
        return FALSE;
    }

    if (DockHostWindow_DeterminePaneKindForHWND(hWnd) == DOCK_PANE_TOOL)
    {
        if (DockHostModelApply_DockToolWindow(pDockHostWindow, hWnd, pTargetHit, iDockSize))
        {
            return TRUE;
        }
    }
    else if (DockHostWindow_DeterminePaneKindForHWND(hWnd) == DOCK_PANE_DOCUMENT)
    {
        if (DockHostModelApply_DockDocumentWindow(pDockHostWindow, hWnd, pTargetHit, iDockSize))
        {
            return TRUE;
        }
    }

    if (pTargetHit->bLocalTarget && pTargetHit->hWndAnchor && IsWindow(pTargetHit->hWndAnchor))
    {
        TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
        TreeNode* pAnchorNode = DockNode_FindByHWND(pRoot, pTargetHit->hWndAnchor);
        DockPaneKind nIncomingPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
        TreeNode* pGroupNode = DockGroup_FindOwningGroup(pRoot, pAnchorNode);
        DockGroupKind nGroupKind = DockGroup_GetNodeKind(pGroupNode);
        if (pAnchorNode && pTargetHit->nDockSide == DKS_CENTER)
        {
            TreeNode* pZoneNode = DockHostWindow_FindOwningZoneNode(pDockHostWindow, pAnchorNode);
            int nZoneSide = DockHostWindow_GetPanelDockSide(pDockHostWindow, pTargetHit->hWndAnchor);
            if (DockGroup_CanTabIntoGroup(nGroupKind, nIncomingPaneKind) &&
                nGroupKind == DOCK_GROUP_TOOL_PANE &&
                pZoneNode && nZoneSide != DKS_NONE &&
                DockHostMutate_DockIntoZone(pDockHostWindow, pZoneNode, hWnd, nZoneSide, iDockSize))
            {
                return TRUE;
            }
        }
        else if (pAnchorNode &&
            DockGroup_CanSplitGroup(nGroupKind, nIncomingPaneKind) &&
            DockHostMutate_DockAroundPanel(pDockHostWindow, pAnchorNode, hWnd, pTargetHit->nDockSide, iDockSize))
        {
            return TRUE;
        }
    }

    return DockHostMutate_DockHWND(pDockHostWindow, hWnd, pTargetHit->nDockSide, iDockSize);
}

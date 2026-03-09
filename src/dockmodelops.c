#include "precomp.h"

#include "dockmodelops.h"

#include "docklayout.h"

static DockModelNode* DockModelOps_CloneNode(const DockModelNode* pNode)
{
	if (!pNode)
	{
		return NULL;
	}

	DockModelNode* pClone = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	if (!pClone)
	{
		return NULL;
	}

	*pClone = *pNode;
	pClone->pChild1 = DockModelOps_CloneNode(pNode->pChild1);
	pClone->pChild2 = DockModelOps_CloneNode(pNode->pChild2);
	return pClone;
}

DockModelNode* DockModelOps_CloneTree(const DockModelNode* pRootNode)
{
	return DockModelOps_CloneNode(pRootNode);
}

DockModelNode* DockModelOps_FindByNodeId(DockModelNode* pRootNode, uint32_t uNodeId)
{
	if (!pRootNode || uNodeId == 0)
	{
		return NULL;
	}

	if (pRootNode->uNodeId == uNodeId)
	{
		return pRootNode;
	}

	DockModelNode* pFound = DockModelOps_FindByNodeId(pRootNode->pChild1, uNodeId);
	if (pFound)
	{
		return pFound;
	}

	return DockModelOps_FindByNodeId(pRootNode->pChild2, uNodeId);
}

DockModelNode* DockModelOps_FindParentByNodeId(DockModelNode* pRootNode, uint32_t uNodeId)
{
	if (!pRootNode || uNodeId == 0)
	{
		return NULL;
	}

	if ((pRootNode->pChild1 && pRootNode->pChild1->uNodeId == uNodeId) ||
		(pRootNode->pChild2 && pRootNode->pChild2->uNodeId == uNodeId))
	{
		return pRootNode;
	}

	DockModelNode* pFound = DockModelOps_FindParentByNodeId(pRootNode->pChild1, uNodeId);
	if (pFound)
	{
		return pFound;
	}

	return DockModelOps_FindParentByNodeId(pRootNode->pChild2, uNodeId);
}

uint32_t DockModelOps_GetMaxNodeId(const DockModelNode* pRootNode)
{
	if (!pRootNode)
	{
		return 0;
	}

	uint32_t uMax = pRootNode->uNodeId;
	uint32_t uLeft = DockModelOps_GetMaxNodeId(pRootNode->pChild1);
	uint32_t uRight = DockModelOps_GetMaxNodeId(pRootNode->pChild2);
	if (uLeft > uMax)
	{
		uMax = uLeft;
	}
	if (uRight > uMax)
	{
		uMax = uRight;
	}
	return uMax;
}

static BOOL DockModelOps_IsSplitRole(DockNodeRole nRole)
{
	return nRole == DOCK_ROLE_SHELL_SPLIT ||
		nRole == DOCK_ROLE_ZONE_STACK_SPLIT ||
		nRole == DOCK_ROLE_PANEL_SPLIT;
}

static void DockModelOps_DestroySingleNode(DockModelNode* pNode)
{
	if (!pNode)
	{
		return;
	}

	pNode->pChild1 = NULL;
	pNode->pChild2 = NULL;
	DockModel_Destroy(pNode);
}

static BOOL DockModelOps_RemoveNodeRecursive(DockModelNode** ppNode, uint32_t uNodeId, BOOL* pbRemoved)
{
	if (!ppNode || !*ppNode || !pbRemoved)
	{
		return FALSE;
	}

	DockModelNode* pNode = *ppNode;
	if (pNode->uNodeId == uNodeId)
	{
		*ppNode = NULL;
		*pbRemoved = TRUE;
		DockModelOps_DestroySingleNode(pNode);
		return TRUE;
	}

	DockModelOps_RemoveNodeRecursive(&pNode->pChild1, uNodeId, pbRemoved);
	DockModelOps_RemoveNodeRecursive(&pNode->pChild2, uNodeId, pbRemoved);

	if (DockModelOps_IsSplitRole(pNode->nRole))
	{
		if (pNode->pChild1 && !pNode->pChild2)
		{
			DockModelNode* pChild = pNode->pChild1;
			pNode->pChild1 = NULL;
			*ppNode = pChild;
			DockModelOps_DestroySingleNode(pNode);
		}
		else if (!pNode->pChild1 && pNode->pChild2)
		{
			DockModelNode* pChild = pNode->pChild2;
			pNode->pChild2 = NULL;
			*ppNode = pChild;
			DockModelOps_DestroySingleNode(pNode);
		}
	}

	return *pbRemoved;
}

BOOL DockModelOps_RemoveNodeById(DockModelNode** ppRootNode, uint32_t uNodeId)
{
	BOOL bRemoved = FALSE;
	if (!ppRootNode || !*ppRootNode || uNodeId == 0)
	{
		return FALSE;
	}

	if ((*ppRootNode)->uNodeId == uNodeId)
	{
		return FALSE;
	}

	return DockModelOps_RemoveNodeRecursive(ppRootNode, uNodeId, &bRemoved);
}

static DockModelNode* DockModelOps_FindZoneBySide(DockModelNode* pNode, int nDockSide)
{
	if (!pNode)
	{
		return NULL;
	}

	if (pNode->nRole == DOCK_ROLE_ZONE && pNode->nDockSide == nDockSide)
	{
		return pNode;
	}

	DockModelNode* pFound = DockModelOps_FindZoneBySide(pNode->pChild1, nDockSide);
	if (pFound)
	{
		return pFound;
	}

	return DockModelOps_FindZoneBySide(pNode->pChild2, nDockSide);
}

static DWORD DockModelOps_GetSplitStyleForSide(int nDockSide)
{
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
        return 0;
    }

    if (nDockSide == DKS_RIGHT || nDockSide == DKS_BOTTOM)
    {
        dwSplitStyle |= DGA_END;
    }
    else {
        dwSplitStyle |= DGA_START;
    }
    return dwSplitStyle;
}

BOOL DockModelOps_AppendPanelToZone(DockModelNode* pRootNode, int nDockSide, DockModelNode* pPanelNode)
{
	if (!pRootNode || !pPanelNode || pPanelNode->nRole != DOCK_ROLE_PANEL)
	{
		return FALSE;
	}

	DockModelNode* pZoneNode = DockModelOps_FindZoneBySide(pRootNode, nDockSide);
	if (!pZoneNode)
	{
		return FALSE;
	}

	uint32_t uNextNodeId = DockModelOps_GetMaxNodeId(pRootNode) + 1;
	if (pPanelNode->uNodeId == 0)
	{
		pPanelNode->uNodeId = uNextNodeId++;
	}

	if (!pZoneNode->pChild1)
	{
		pZoneNode->pChild1 = pPanelNode;
		return TRUE;
	}

	DockModelNode* pSplit = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	if (!pSplit)
	{
		return FALSE;
	}

	pSplit->uNodeId = uNextNodeId;
	pSplit->nRole = DOCK_ROLE_ZONE_STACK_SPLIT;
	pSplit->dwStyle = DockLayout_GetZoneStackStyle(nDockSide);
	pSplit->iGripPos = DockLayout_GetZoneStackGrip(nDockSide, 280);
	wcscpy_s(pSplit->szName, ARRAYSIZE(pSplit->szName), L"DockShell.ZoneStack");
	pSplit->pChild1 = pZoneNode->pChild1;
	pSplit->pChild2 = pPanelNode;
	pZoneNode->pChild1 = pSplit;
	return TRUE;
}

BOOL DockModelOps_DockPanelAroundNode(DockModelNode* pRootNode, uint32_t uAnchorNodeId, int nDockSide, DockModelNode* pPanelNode)
{
    if (!pRootNode || !pPanelNode || pPanelNode->nRole != DOCK_ROLE_PANEL || uAnchorNodeId == 0)
    {
        return FALSE;
    }

    DockModelNode* pAnchorNode = DockModelOps_FindByNodeId(pRootNode, uAnchorNodeId);
    if (!pAnchorNode)
    {
        return FALSE;
    }

    DockModelNode* pParentNode = DockModelOps_FindParentByNodeId(pRootNode, uAnchorNodeId);
    DWORD dwSplitStyle = DockModelOps_GetSplitStyleForSide(nDockSide);
    if (dwSplitStyle == 0)
    {
        return FALSE;
    }

    uint32_t uNextNodeId = DockModelOps_GetMaxNodeId(pRootNode) + 1;
    if (pPanelNode->uNodeId == 0)
    {
        pPanelNode->uNodeId = uNextNodeId++;
    }

    DockModelNode* pSplit = (DockModelNode*)calloc(1, sizeof(DockModelNode));
    if (!pSplit)
    {
        return FALSE;
    }

    pSplit->uNodeId = uNextNodeId;
    pSplit->nRole = DOCK_ROLE_PANEL_SPLIT;
    pSplit->dwStyle = dwSplitStyle;
    pSplit->iGripPos = DockLayout_GetZoneSplitGrip(nDockSide, 280);
    pSplit->fGripPos = -1.0f;
    wcscpy_s(pSplit->szName, ARRAYSIZE(pSplit->szName), L"DockShell.PanelSplit");

    if (nDockSide == DKS_LEFT || nDockSide == DKS_TOP)
    {
        pSplit->pChild1 = pPanelNode;
        pSplit->pChild2 = pAnchorNode;
    }
    else {
        pSplit->pChild1 = pAnchorNode;
        pSplit->pChild2 = pPanelNode;
    }

    if (!pParentNode)
    {
        DockModelOps_DestroySingleNode(pSplit);
        return FALSE;
    }

    if (pParentNode->pChild1 == pAnchorNode)
    {
        pParentNode->pChild1 = pSplit;
    }
    else if (pParentNode->pChild2 == pAnchorNode)
    {
        pParentNode->pChild2 = pSplit;
    }
    else {
        DockModelOps_DestroySingleNode(pSplit);
        return FALSE;
    }

    return TRUE;
}

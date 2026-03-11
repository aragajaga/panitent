#include "precomp.h"

#include "dockmodelmatch.h"

#include "dockmodelops.h"
#include "dockviewcatalog.h"

DockModelNode* DockModelMatch_FindByViewId(DockModelNode* pNode, PanitentDockViewId nViewId)
{
    if (!pNode || nViewId == PNT_DOCK_VIEW_NONE)
    {
        return NULL;
    }

    if ((PanitentDockViewId)pNode->uViewId == nViewId)
    {
        return pNode;
    }

    DockModelNode* pFound = DockModelMatch_FindByViewId(pNode->pChild1, nViewId);
    if (pFound)
    {
        return pFound;
    }

    return DockModelMatch_FindByViewId(pNode->pChild2, nViewId);
}

DockModelNode* DockModelMatch_FindByName(DockModelNode* pNode, PCWSTR pszName)
{
    if (!pNode || !pszName || !pszName[0])
    {
        return NULL;
    }

    if (wcscmp(pNode->szName, pszName) == 0)
    {
        return pNode;
    }

    DockModelNode* pFound = DockModelMatch_FindByName(pNode->pChild1, pszName);
    if (pFound)
    {
        return pFound;
    }

    return DockModelMatch_FindByName(pNode->pChild2, pszName);
}

static BOOL DockModelMatch_GetLiveWorkspaceOrdinalRecursive(
    const TreeNode* pNode,
    HWND hWndTarget,
    int* pnOrdinal,
    BOOL* pbFound)
{
    if (!pNode || !pNode->data || !pnOrdinal || !pbFound || *pbFound)
    {
        return *pbFound;
    }

    const DockData* pDockData = (const DockData*)pNode->data;
    if (pDockData->nRole == DOCK_ROLE_WORKSPACE)
    {
        if (pDockData->hWnd == hWndTarget)
        {
            *pbFound = TRUE;
            return TRUE;
        }

        (*pnOrdinal)++;
    }

    DockModelMatch_GetLiveWorkspaceOrdinalRecursive(pNode->node1, hWndTarget, pnOrdinal, pbFound);
    DockModelMatch_GetLiveWorkspaceOrdinalRecursive(pNode->node2, hWndTarget, pnOrdinal, pbFound);
    return *pbFound;
}

static DockModelNode* DockModelMatch_FindWorkspaceByOrdinalRecursive(
    DockModelNode* pNode,
    int nTargetOrdinal,
    int* pnCurrentOrdinal)
{
    if (!pNode || !pnCurrentOrdinal)
    {
        return NULL;
    }

    if (pNode->nRole == DOCK_ROLE_WORKSPACE)
    {
        if (*pnCurrentOrdinal == nTargetOrdinal)
        {
            return pNode;
        }

        (*pnCurrentOrdinal)++;
    }

    DockModelNode* pFound = DockModelMatch_FindWorkspaceByOrdinalRecursive(
        pNode->pChild1,
        nTargetOrdinal,
        pnCurrentOrdinal);
    if (pFound)
    {
        return pFound;
    }

    return DockModelMatch_FindWorkspaceByOrdinalRecursive(
        pNode->pChild2,
        nTargetOrdinal,
        pnCurrentOrdinal);
}

DockModelNode* DockModelMatch_FindWorkspaceByLiveHwnd(const TreeNode* pLiveRoot, DockModelNode* pModelRoot, HWND hWndWorkspace)
{
    int nOrdinal = 0;
    BOOL bFound = FALSE;
    if (!pLiveRoot || !pModelRoot || !hWndWorkspace)
    {
        return NULL;
    }

    if (!DockModelMatch_GetLiveWorkspaceOrdinalRecursive(pLiveRoot, hWndWorkspace, &nOrdinal, &bFound) || !bFound)
    {
        return NULL;
    }

    int nCurrentOrdinal = 0;
    return DockModelMatch_FindWorkspaceByOrdinalRecursive(pModelRoot, nOrdinal, &nCurrentOrdinal);
}

DockModelNode* DockModelMatch_FindNodeForLiveData(const TreeNode* pLiveRoot, DockModelNode* pModelRoot, const DockData* pDockData)
{
    if (!pModelRoot || !pDockData)
    {
        return NULL;
    }

    if (pDockData->uModelNodeId != 0)
    {
        DockModelNode* pById = DockModelOps_FindByNodeId(pModelRoot, pDockData->uModelNodeId);
        if (pById)
        {
            return pById;
        }
    }

    if (pDockData->nRole == DOCK_ROLE_WORKSPACE && pDockData->hWnd && IsWindow(pDockData->hWnd))
    {
        DockModelNode* pWorkspaceNode = DockModelMatch_FindWorkspaceByLiveHwnd(pLiveRoot, pModelRoot, pDockData->hWnd);
        if (pWorkspaceNode)
        {
            return pWorkspaceNode;
        }
    }

    PanitentDockViewId nViewId = pDockData->nViewId != PNT_DOCK_VIEW_NONE ?
        pDockData->nViewId :
        PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName);
    if (nViewId != PNT_DOCK_VIEW_NONE)
    {
        DockModelNode* pByViewId = DockModelMatch_FindByViewId(pModelRoot, nViewId);
        if (pByViewId)
        {
            return pByViewId;
        }
    }

    return DockModelMatch_FindByName(pModelRoot, pDockData->lpszName);
}

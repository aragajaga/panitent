#include "precomp.h"

#include "dockhostmodelapply.h"

#include "dockhostrestore.h"
#include "dockhosttree.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockmodelops.h"
#include "dockmodelvalidate.h"
#include "dockviewcatalog.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"

typedef struct DockHostModelApplyViewEntry
{
    PanitentDockViewId nViewId;
    uint32_t uModelNodeId;
    HWND hWnd;
    BOOL bMainWorkspace;
} DockHostModelApplyViewEntry;

typedef struct DockHostModelApplyContext
{
    PanitentApp* pPanitentApp;
    DockHostModelApplyViewEntry entries[16];
    int nEntries;
} DockHostModelApplyContext;

static DockModelNode* DockHostModelApply_FindModelNodeByViewId(DockModelNode* pNode, PanitentDockViewId nViewId)
{
    if (!pNode || nViewId == PNT_DOCK_VIEW_NONE)
    {
        return NULL;
    }

    if ((PanitentDockViewId)pNode->uViewId == nViewId)
    {
        return pNode;
    }

    DockModelNode* pFound = DockHostModelApply_FindModelNodeByViewId(pNode->pChild1, nViewId);
    if (pFound)
    {
        return pFound;
    }

    return DockHostModelApply_FindModelNodeByViewId(pNode->pChild2, nViewId);
}

static BOOL DockHostModelApply_IsWorkspaceHwnd(HWND hWnd)
{
    WCHAR szClassName[64] = L"";
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return wcscmp(szClassName, L"__WorkspaceContainer") == 0;
}

static BOOL DockHostModelApply_GetLiveWorkspaceOrdinalRecursive(
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

    DockHostModelApply_GetLiveWorkspaceOrdinalRecursive(pNode->node1, hWndTarget, pnOrdinal, pbFound);
    DockHostModelApply_GetLiveWorkspaceOrdinalRecursive(pNode->node2, hWndTarget, pnOrdinal, pbFound);
    return *pbFound;
}

static DockModelNode* DockHostModelApply_FindWorkspaceByOrdinalRecursive(
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

    DockModelNode* pFound = DockHostModelApply_FindWorkspaceByOrdinalRecursive(
        pNode->pChild1,
        nTargetOrdinal,
        pnCurrentOrdinal);
    if (pFound)
    {
        return pFound;
    }

    return DockHostModelApply_FindWorkspaceByOrdinalRecursive(
        pNode->pChild2,
        nTargetOrdinal,
        pnCurrentOrdinal);
}

static DockModelNode* DockHostModelApply_FindWorkspaceByLiveHwnd(
    const TreeNode* pLiveRoot,
    DockModelNode* pModelRoot,
    HWND hWndWorkspace)
{
    int nOrdinal = 0;
    BOOL bFound = FALSE;
    if (!pLiveRoot || !pModelRoot || !hWndWorkspace)
    {
        return NULL;
    }

    if (!DockHostModelApply_GetLiveWorkspaceOrdinalRecursive(pLiveRoot, hWndWorkspace, &nOrdinal, &bFound) || !bFound)
    {
        return NULL;
    }

    int nCurrentOrdinal = 0;
    return DockHostModelApply_FindWorkspaceByOrdinalRecursive(pModelRoot, nOrdinal, &nCurrentOrdinal);
}

static DockModelNode* DockHostModelApply_FindModelNodeByName(DockModelNode* pNode, PCWSTR pszName)
{
    if (!pNode || !pszName || !pszName[0])
    {
        return NULL;
    }

    if (wcscmp(pNode->szName, pszName) == 0)
    {
        return pNode;
    }

    DockModelNode* pFound = DockHostModelApply_FindModelNodeByName(pNode->pChild1, pszName);
    if (pFound)
    {
        return pFound;
    }

    return DockHostModelApply_FindModelNodeByName(pNode->pChild2, pszName);
}

static DockModelNode* DockHostModelApply_FindModelNodeForLiveData(
    const TreeNode* pLiveRoot,
    DockModelNode* pRoot,
    const DockData* pDockData)
{
    if (!pRoot || !pDockData)
    {
        return NULL;
    }

    if (pDockData->uModelNodeId != 0)
    {
        DockModelNode* pById = DockModelOps_FindByNodeId(pRoot, pDockData->uModelNodeId);
        if (pById)
        {
            return pById;
        }
    }

    if (pDockData->nRole == DOCK_ROLE_WORKSPACE && pDockData->hWnd && IsWindow(pDockData->hWnd))
    {
        DockModelNode* pWorkspaceNode = DockHostModelApply_FindWorkspaceByLiveHwnd(pLiveRoot, pRoot, pDockData->hWnd);
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
        DockModelNode* pByViewId = DockHostModelApply_FindModelNodeByViewId(pRoot, nViewId);
        if (pByViewId)
        {
            return pByViewId;
        }
    }

    return DockHostModelApply_FindModelNodeByName(pRoot, pDockData->lpszName);
}

static PanitentDockViewId DockHostModelApply_GetViewIdForHwnd(HWND hWnd)
{
    WCHAR szTitle[128] = L"";
    WCHAR szClassName[64] = L"";
    if (!hWnd || !IsWindow(hWnd))
    {
        return PNT_DOCK_VIEW_NONE;
    }

    GetWindowTextW(hWnd, szTitle, ARRAYSIZE(szTitle));
    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return PanitentDockViewCatalog_FindForWindow(szClassName, szTitle);
}

static BOOL DockHostModelApply_AddPreservedView(
    DockHostModelApplyContext* pContext,
    PanitentDockViewId nViewId,
    uint32_t uModelNodeId,
    HWND hWnd,
    BOOL bMainWorkspace)
{
    if (!pContext || nViewId == PNT_DOCK_VIEW_NONE || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    for (int i = 0; i < pContext->nEntries; ++i)
    {
        if (pContext->entries[i].hWnd == hWnd)
        {
            return TRUE;
        }

        if (nViewId == PNT_DOCK_VIEW_WORKSPACE)
        {
            if (uModelNodeId != 0 && pContext->entries[i].uModelNodeId == uModelNodeId)
            {
                return TRUE;
            }
        }
        else if (pContext->entries[i].nViewId == nViewId)
        {
            return TRUE;
        }
    }

    if (pContext->nEntries >= ARRAYSIZE(pContext->entries))
    {
        return FALSE;
    }

    pContext->entries[pContext->nEntries].nViewId = nViewId;
    pContext->entries[pContext->nEntries].uModelNodeId = uModelNodeId;
    pContext->entries[pContext->nEntries].hWnd = hWnd;
    pContext->entries[pContext->nEntries].bMainWorkspace = bMainWorkspace;
    pContext->nEntries++;
    return TRUE;
}

static void DockHostModelApply_CollectViewsRecursive(
    DockHostModelApplyContext* pContext,
    const TreeNode* pLiveRoot,
    DockModelNode* pModelRoot,
    TreeNode* pNode)
{
    if (!pContext || !pNode || !pNode->data)
    {
        return;
    }

    DockData* pDockData = (DockData*)pNode->data;
    PanitentDockViewId nViewId = pDockData->nViewId != PNT_DOCK_VIEW_NONE ?
        pDockData->nViewId :
        PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName);
    if (nViewId != PNT_DOCK_VIEW_NONE && pDockData->hWnd && IsWindow(pDockData->hWnd))
    {
        DockModelNode* pModelNode = DockHostModelApply_FindModelNodeForLiveData(pLiveRoot, pModelRoot, pDockData);
        DockHostModelApply_AddPreservedView(
            pContext,
            nViewId,
            pModelNode ? pModelNode->uNodeId : 0,
            pDockData->hWnd,
            nViewId == PNT_DOCK_VIEW_WORKSPACE &&
                pContext->pPanitentApp &&
                pContext->pPanitentApp->m_pWorkspaceContainer &&
                pDockData->hWnd == Window_GetHWND((Window*)pContext->pPanitentApp->m_pWorkspaceContainer));
    }

    DockHostModelApply_CollectViewsRecursive(pContext, pLiveRoot, pModelRoot, pNode->node1);
    DockHostModelApply_CollectViewsRecursive(pContext, pLiveRoot, pModelRoot, pNode->node2);
}

static void DockHostModelApply_CollectViewsRecursiveEx(
    DockHostModelApplyContext* pContext,
    const TreeNode* pLiveRoot,
    DockModelNode* pModelRoot,
    TreeNode* pNode,
    HWND hWndIncludeHidden)
{
    if (!pContext || !pNode || !pNode->data)
    {
        return;
    }

    DockData* pDockData = (DockData*)pNode->data;
    PanitentDockViewId nViewId = pDockData->nViewId != PNT_DOCK_VIEW_NONE ?
        pDockData->nViewId :
        PanitentDockViewCatalog_Find(pDockData->nRole, pDockData->lpszName);
    if (nViewId != PNT_DOCK_VIEW_NONE && pDockData->hWnd && IsWindow(pDockData->hWnd))
    {
        DockModelNode* pModelNode = DockHostModelApply_FindModelNodeForLiveData(pLiveRoot, pModelRoot, pDockData);
        DockHostModelApply_AddPreservedView(
            pContext,
            nViewId,
            pModelNode ? pModelNode->uNodeId : 0,
            pDockData->hWnd,
            nViewId == PNT_DOCK_VIEW_WORKSPACE &&
                pContext->pPanitentApp &&
                pContext->pPanitentApp->m_pWorkspaceContainer &&
                pDockData->hWnd == Window_GetHWND((Window*)pContext->pPanitentApp->m_pWorkspaceContainer));
    }
    else if (hWndIncludeHidden && pDockData->hWnd == hWndIncludeHidden)
    {
        DockHostModelApply_AddPreservedView(
            pContext,
            DockHostModelApply_GetViewIdForHwnd(hWndIncludeHidden),
            0,
            hWndIncludeHidden,
            FALSE);
    }

    DockHostModelApply_CollectViewsRecursiveEx(pContext, pLiveRoot, pModelRoot, pNode->node1, hWndIncludeHidden);
    DockHostModelApply_CollectViewsRecursiveEx(pContext, pLiveRoot, pModelRoot, pNode->node2, hWndIncludeHidden);
}

static Window* DockHostModelApply_ResolveView(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    PanitentDockViewId nViewId,
    void* pUserData)
{
    UNREFERENCED_PARAMETER(pDockHostWindow);
    UNREFERENCED_PARAMETER(pNode);
    UNREFERENCED_PARAMETER(pDockData);

    DockHostModelApplyContext* pContext = (DockHostModelApplyContext*)pUserData;
    if (!pContext)
    {
        return NULL;
    }

    for (int i = 0; i < pContext->nEntries; ++i)
    {
        if (nViewId == PNT_DOCK_VIEW_WORKSPACE)
        {
            if (pDockData &&
                pDockData->uModelNodeId != 0 &&
                pContext->entries[i].uModelNodeId == pDockData->uModelNodeId)
            {
                Window* pWindow = (Window*)WindowMap_Get(pContext->entries[i].hWnd);
                if (pContext->entries[i].bMainWorkspace && pWindow)
                {
                    pPanitentApp->m_pWorkspaceContainer = (WorkspaceContainer*)pWindow;
                }
                return pWindow;
            }

            continue;
        }

        if (pContext->entries[i].nViewId == nViewId)
        {
            Window* pWindow = (Window*)WindowMap_Get(pContext->entries[i].hWnd);
            if (pContext->entries[i].bMainWorkspace && pWindow)
            {
                pPanitentApp->m_pWorkspaceContainer = (WorkspaceContainer*)pWindow;
            }
            return pWindow;
        }
    }

    return NULL;
}

static void DockHostModelApply_FillPreserveArray(const DockHostModelApplyContext* pContext, HWND* phWnds, int cHwnds, int* pnHwnds)
{
    if (!phWnds || cHwnds <= 0 || !pnHwnds)
    {
        return;
    }

    *pnHwnds = 0;
    if (!pContext)
    {
        return;
    }

    for (int i = 0; i < pContext->nEntries && *pnHwnds < cHwnds; ++i)
    {
        phWnds[*pnHwnds] = pContext->entries[i].hWnd;
        (*pnHwnds)++;
    }
}

static DockModelNode* DockHostModelApply_CreatePanelModel(HWND hWnd)
{
    PanitentDockViewId nViewId = DockHostModelApply_GetViewIdForHwnd(hWnd);
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
    if (!DockHostModelApply_IsWorkspaceHwnd(hWnd))
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
    DockHostModelApply_FillPreserveArray(pContext, hPreserve, ARRAYSIZE(hPreserve), &nPreserve);
    DockHostWindow_ClearLayout(pDockHostWindow, hPreserve, nPreserve);

    BOOL bHasWorkspace = FALSE;
    if (!PanitentDockHostRestoreAttachKnownViewsEx(
        pContext->pPanitentApp,
        pDockHostWindow,
        pRootNode,
        DockHostModelApply_ResolveView,
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
    DockHostModelApply_CollectViewsRecursive(
        &context,
        DockHostWindow_GetRoot(pDockHostWindow),
        pRollbackModel,
        DockHostWindow_GetRoot(pDockHostWindow));
    DockHostModelApply_AddPreservedView(
        &context,
        DockHostModelApply_GetViewIdForHwnd(hWnd),
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
            DockModelNode* pAnchorModelNode = DockHostModelApply_FindModelNodeForLiveData(
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
    DockHostModelApply_CollectViewsRecursiveEx(
        &context,
        DockHostWindow_GetRoot(pDockHostWindow),
        pRollbackModel,
        DockHostWindow_GetRoot(pDockHostWindow),
        hWnd);

    DockModelNode* pLiveModelNode = DockHostModelApply_FindModelNodeForLiveData(
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

    if (!pDockHostWindow || !hWnd || !IsWindow(hWnd) || !DockHostModelApply_IsWorkspaceHwnd(hWnd) ||
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
    DockHostModelApply_CollectViewsRecursive(&context, pLiveRoot, pRollbackModel, pLiveRoot);

    DockModelNode* pAnchorModelNode = DockHostModelApply_FindModelNodeForLiveData(
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

    DockHostModelApply_AddPreservedView(
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

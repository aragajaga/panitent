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
    HWND hWnd;
} DockHostModelApplyViewEntry;

typedef struct DockHostModelApplyContext
{
    PanitentApp* pPanitentApp;
    DockHostModelApplyViewEntry entries[16];
    int nEntries;
} DockHostModelApplyContext;

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

static BOOL DockHostModelApply_AddPreservedView(DockHostModelApplyContext* pContext, PanitentDockViewId nViewId, HWND hWnd)
{
    if (!pContext || nViewId == PNT_DOCK_VIEW_NONE || !hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    for (int i = 0; i < pContext->nEntries; ++i)
    {
        if (pContext->entries[i].nViewId == nViewId || pContext->entries[i].hWnd == hWnd)
        {
            return TRUE;
        }
    }

    if (pContext->nEntries >= ARRAYSIZE(pContext->entries))
    {
        return FALSE;
    }

    pContext->entries[pContext->nEntries].nViewId = nViewId;
    pContext->entries[pContext->nEntries].hWnd = hWnd;
    pContext->nEntries++;
    return TRUE;
}

static void DockHostModelApply_CollectViewsRecursive(DockHostModelApplyContext* pContext, TreeNode* pNode)
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
        DockHostModelApply_AddPreservedView(pContext, nViewId, pDockData->hWnd);
    }

    DockHostModelApply_CollectViewsRecursive(pContext, pNode->node1);
    DockHostModelApply_CollectViewsRecursive(pContext, pNode->node2);
}

static void DockHostModelApply_CollectViewsRecursiveEx(DockHostModelApplyContext* pContext, TreeNode* pNode, HWND hWndIncludeHidden)
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
        DockHostModelApply_AddPreservedView(pContext, nViewId, pDockData->hWnd);
    }
    else if (hWndIncludeHidden && pDockData->hWnd == hWndIncludeHidden)
    {
        DockHostModelApply_AddPreservedView(pContext, DockHostModelApply_GetViewIdForHwnd(hWndIncludeHidden), hWndIncludeHidden);
    }

    DockHostModelApply_CollectViewsRecursiveEx(pContext, pNode->node1, hWndIncludeHidden);
    DockHostModelApply_CollectViewsRecursiveEx(pContext, pNode->node2, hWndIncludeHidden);
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
        if (pContext->entries[i].nViewId == nViewId)
        {
            Window* pWindow = (Window*)WindowMap_Get(pContext->entries[i].hWnd);
            if (nViewId == PNT_DOCK_VIEW_WORKSPACE && pWindow)
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

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostModelApply_CollectViewsRecursive(&context, DockHostWindow_GetRoot(pDockHostWindow));
    DockHostModelApply_AddPreservedView(&context, DockHostModelApply_GetViewIdForHwnd(hWnd), hWnd);

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

    BOOL bMutated = FALSE;
    if (pTargetHit->bLocalTarget && pTargetHit->hWndAnchor && IsWindow(pTargetHit->hWndAnchor))
    {
        TreeNode* pAnchorLiveNode = DockNode_FindByHWND(DockHostWindow_GetRoot(pDockHostWindow), pTargetHit->hWndAnchor);
        DockData* pAnchorLiveData = pAnchorLiveNode ? (DockData*)pAnchorLiveNode->data : NULL;
        if (pAnchorLiveData)
        {
            if (pTargetHit->nDockSide == DKS_CENTER)
            {
                int nZoneSide = DockHostWindow_GetPanelDockSide(pDockHostWindow, pTargetHit->hWndAnchor);
                bMutated = DockModelOps_AppendPanelToZone(pTargetModel, nZoneSide, pPanelNode);
            }
            else {
                bMutated = DockModelOps_DockPanelAroundNode(pTargetModel, pAnchorLiveData->uModelNodeId, pTargetHit->nDockSide, pPanelNode);
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

    DockHostModelApplyContext context = { 0 };
    context.pPanitentApp = PanitentApp_Instance();
    DockHostModelApply_CollectViewsRecursiveEx(&context, DockHostWindow_GetRoot(pDockHostWindow), hWnd);

    DockModelNode* pRollbackModel = DockModel_CaptureHostLayout(pDockHostWindow);
    DockModelNode* pTargetModel = DockModelOps_CloneTree(pRollbackModel);
    if (!pRollbackModel || !pTargetModel)
    {
        DockModel_Destroy(pRollbackModel);
        DockModel_Destroy(pTargetModel);
        return FALSE;
    }

    if (!DockModelOps_RemoveNodeById(&pTargetModel, pLiveData->uModelNodeId) ||
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

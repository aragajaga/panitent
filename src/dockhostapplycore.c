#include "precomp.h"

#include "dockhostapplycore.h"

#include "dockhostrestore.h"
#include "dockmodelbuild.h"
#include "dockviewcatalog.h"
#include "win32/window.h"

DockModelNode* DockHostApplyCore_CreatePanelModel(HWND hWnd)
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

DockModelNode* DockHostApplyCore_CreateWorkspaceModel(HWND hWnd)
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

BOOL DockHostApplyCore_ApplyModel(DockHostWindow* pDockHostWindow, DockModelNode* pModelRoot, DockHostModelApplyContext* pContext)
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

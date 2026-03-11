#include "precomp.h"

#include "dockhostpreserve.h"

#include "dockmodelmatch.h"
#include "dockviewcatalog.h"
#include "panitentapp.h"
#include "win32/window.h"
#include "win32/windowmap.h"

BOOL DockHostPreserve_IsWorkspaceHwnd(HWND hWnd)
{
    WCHAR szClassName[64] = L"";
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return wcscmp(szClassName, L"__WorkspaceContainer") == 0;
}

PanitentDockViewId DockHostPreserve_GetViewIdForHwnd(HWND hWnd)
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

BOOL DockHostPreserve_AddView(
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

void DockHostPreserve_CollectViewsRecursive(
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
        DockModelNode* pModelNode = DockModelMatch_FindNodeForLiveData(pLiveRoot, pModelRoot, pDockData);
        DockHostPreserve_AddView(
            pContext,
            nViewId,
            pModelNode ? pModelNode->uNodeId : 0,
            pDockData->hWnd,
            nViewId == PNT_DOCK_VIEW_WORKSPACE &&
                pContext->pPanitentApp &&
                pContext->pPanitentApp->m_pWorkspaceContainer &&
                pDockData->hWnd == Window_GetHWND((Window*)pContext->pPanitentApp->m_pWorkspaceContainer));
    }

    DockHostPreserve_CollectViewsRecursive(pContext, pLiveRoot, pModelRoot, pNode->node1);
    DockHostPreserve_CollectViewsRecursive(pContext, pLiveRoot, pModelRoot, pNode->node2);
}

void DockHostPreserve_CollectViewsRecursiveEx(
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
        DockModelNode* pModelNode = DockModelMatch_FindNodeForLiveData(pLiveRoot, pModelRoot, pDockData);
        DockHostPreserve_AddView(
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
        DockHostPreserve_AddView(
            pContext,
            DockHostPreserve_GetViewIdForHwnd(hWndIncludeHidden),
            0,
            hWndIncludeHidden,
            FALSE);
    }

    DockHostPreserve_CollectViewsRecursiveEx(pContext, pLiveRoot, pModelRoot, pNode->node1, hWndIncludeHidden);
    DockHostPreserve_CollectViewsRecursiveEx(pContext, pLiveRoot, pModelRoot, pNode->node2, hWndIncludeHidden);
}

Window* DockHostPreserve_ResolveView(
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

void DockHostPreserve_FillPreserveArray(const DockHostModelApplyContext* pContext, HWND* phWnds, int cHwnds, int* pnHwnds)
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

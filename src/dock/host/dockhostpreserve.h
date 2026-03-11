#pragma once

#include "dockhost.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockModelNode DockModelNode;

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

BOOL DockHostPreserve_IsWorkspaceHwnd(HWND hWnd);
PanitentDockViewId DockHostPreserve_GetViewIdForHwnd(HWND hWnd);
BOOL DockHostPreserve_AddView(
    DockHostModelApplyContext* pContext,
    PanitentDockViewId nViewId,
    uint32_t uModelNodeId,
    HWND hWnd,
    BOOL bMainWorkspace);
void DockHostPreserve_CollectViewsRecursive(
    DockHostModelApplyContext* pContext,
    const TreeNode* pLiveRoot,
    DockModelNode* pModelRoot,
    TreeNode* pNode);
void DockHostPreserve_CollectViewsRecursiveEx(
    DockHostModelApplyContext* pContext,
    const TreeNode* pLiveRoot,
    DockModelNode* pModelRoot,
    TreeNode* pNode,
    HWND hWndIncludeHidden);
Window* DockHostPreserve_ResolveView(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    PanitentDockViewId nViewId,
    void* pUserData);
void DockHostPreserve_FillPreserveArray(const DockHostModelApplyContext* pContext, HWND* phWnds, int cHwnds, int* pnHwnds);

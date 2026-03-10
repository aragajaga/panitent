#pragma once

#include "floatingdocumenthost.h"

DockModelNode* FloatingDocumentHost_CaptureChildLayout(HWND hWndChild);
BOOL FloatingDocumentHost_IsPinnedFloatingWindow(HWND hWnd, FloatingWindowContainer** ppFloatingWindowContainer);
BOOL FloatingDocumentHost_ForEachPinnedWindow(FnFloatingDocumentHostWindowCallback pfnCallback, void* pUserData);
BOOL FloatingDocumentHost_CapturePinnedWindowState(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    RECT* pWindowRect,
    DockModelNode** ppLayoutModel,
    HWND* pWorkspaceHwnds,
    int cWorkspaceHwnds,
    int* pnWorkspaceCount);
BOOL FloatingDocumentHost_CollectLiveWorkspaces(FloatingDocumentWorkspaceReuseContext* pContext);
BOOL FloatingDocumentHost_PrepareWorkspaceReuse(
    FloatingDocumentWorkspaceReuseContext* pContext,
    BOOL bClearViewports);
Window* FloatingDocumentHost_ResolveReusedWorkspace(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    PanitentDockViewId nViewId,
    void* pUserData);
void FloatingDocumentHost_DisposeUnusedReusedWorkspaces(
    PanitentApp* pPanitentApp,
    FloatingDocumentWorkspaceReuseContext* pContext);

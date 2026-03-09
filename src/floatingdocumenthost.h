#pragma once

#include "dockhost.h"
typedef struct DockModelNode DockModelNode;
typedef struct Window Window;
typedef struct FloatingWindowContainer FloatingWindowContainer;

typedef struct PanitentApp PanitentApp;
typedef BOOL (*FnFloatingDocumentHostCreatePinnedWindowHook)(
    DockHostWindow* pDockHostTarget,
    HWND hWndChild,
    const RECT* pWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut);
typedef BOOL (*FnFloatingDocumentHostWindowCallback)(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    void* pUserData);
typedef Window* (*FnDockHostRestoreResolveView)(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    PanitentDockViewId nViewId,
    void* pUserData);
typedef BOOL (*FnDockHostRestoreNodeAttached)(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    TreeNode* pNode,
    DockData* pDockData,
    Window* pWindow,
    void* pUserData);

BOOL FloatingDocumentHost_CreatePinnedWindow(
    DockHostWindow* pDockHostTarget,
    HWND hWndChild,
    const RECT* pWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut);

BOOL FloatingDocumentHost_CreatePinnedDockHost(
    DockHostWindow* pDockHostTarget,
    PanitentApp* pPanitentApp,
    const RECT* pWindowRect,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndDockHostOut,
    HWND* phWndFloatingOut);

BOOL FloatingDocumentHost_RestorePinnedDockHost(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostTarget,
    const RECT* pWindowRect,
    const DockModelNode* pLayoutModel,
    FnDockHostRestoreResolveView pfnResolveView,
    void* pResolveViewUserData,
    FnDockHostRestoreNodeAttached pfnNodeAttached,
    void* pNodeAttachedUserData,
    BOOL* pbHasWorkspace,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndFloatingOut);

void FloatingDocumentHost_SetCreatePinnedWindowTestHook(FnFloatingDocumentHostCreatePinnedWindowHook pfnHook);
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

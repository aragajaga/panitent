#pragma once

#include "dockhost.h"
typedef struct DockModelNode DockModelNode;
typedef struct Window Window;

typedef struct PanitentApp PanitentApp;
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

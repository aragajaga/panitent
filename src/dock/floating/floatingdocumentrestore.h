#pragma once

#include "floatingdocumenthost.h"

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
BOOL FloatingDocumentHost_RestorePinnedDockHostWithReuse(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostTarget,
    const RECT* pWindowRect,
    const DockModelNode* pLayoutModel,
    FloatingDocumentWorkspaceReuseContext* pReuse,
    FnDockHostRestoreNodeAttached pfnNodeAttached,
    void* pNodeAttachedUserData,
    BOOL* pbHasWorkspace,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndFloatingOut);

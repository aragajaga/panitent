#pragma once

#include "dockhost.h"
#include "dockhostrestore.h"

typedef struct DockModelNode DockModelNode;
typedef struct Window Window;
typedef struct FloatingWindowContainer FloatingWindowContainer;
typedef struct WorkspaceContainer WorkspaceContainer;

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

typedef struct FloatingDocumentWorkspaceReuseContext
{
    HWND hWorkspaceHwnds[32];
    int nWorkspaceCount;
    int iNextWorkspace;
} FloatingDocumentWorkspaceReuseContext;

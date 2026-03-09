#pragma once

#include "precomp.h"

#include "floatingdockpolicy.h"

typedef struct WorkspaceContainer WorkspaceContainer;
typedef struct DockHostWindow DockHostWindow;
typedef struct ViewportWindow ViewportWindow;

#define FLOATING_CHILDHOST_SIDE_DOCK_MAX_WORKSPACES 16
#define FLOATING_CHILDHOST_SIDE_DOCK_MAX_VIEWPORTS 32

typedef struct FloatingChildHostSideDockWorkspaceState
{
    HWND hWndWorkspace;
    WorkspaceContainer* pWorkspace;
    ViewportWindow* pActiveViewport;
    int nViewportCount;
    ViewportWindow* pViewports[FLOATING_CHILDHOST_SIDE_DOCK_MAX_VIEWPORTS];
} FloatingChildHostSideDockWorkspaceState;

typedef struct FloatingChildHostSideDockPrep
{
    HWND hWndOriginalChild;
    HWND hWndPreparedChild;
    HWND hWndFloating;
    ViewportWindow* pAppActiveViewport;
    int nWorkspaceCount;
    FloatingChildHostSideDockWorkspaceState workspaces[FLOATING_CHILDHOST_SIDE_DOCK_MAX_WORKSPACES];
} FloatingChildHostSideDockPrep;

FloatingDockChildHostKind FloatingChildHost_GetKind(HWND hWndChild);
int FloatingChildHost_CollectDocumentWorkspaceHwnds(HWND hWndChild, HWND* pWorkspaceHwnds, int cWorkspaceHwnds);
BOOL FloatingChildHost_GetDocumentSourceContext(HWND hWndChild, POINT ptScreen, WorkspaceContainer** ppWorkspaceSource, int* pnSourceDocumentCount);
BOOL FloatingChildHost_MoveDocumentsToWorkspace(HWND hWndChild, WorkspaceContainer* pWorkspaceTarget);
BOOL FloatingChildHost_EnsureWorkspaceChildForSideDock(HWND hWndFloating, HWND* phWndChild);
BOOL FloatingChildHost_PrepareWorkspaceChildForSideDock(HWND hWndFloating, HWND* phWndChild, FloatingChildHostSideDockPrep* pPrep);
void FloatingChildHost_CommitWorkspaceChildForSideDock(FloatingChildHostSideDockPrep* pPrep);
void FloatingChildHost_RollbackWorkspaceChildForSideDock(FloatingChildHostSideDockPrep* pPrep, HWND* phWndChild);
DockHostWindow* FloatingChildHost_EnsureTargetWorkspaceDockHost(HWND hWndTargetWorkspace);

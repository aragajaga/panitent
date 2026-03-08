#pragma once

#include "precomp.h"

#include "floatingdockpolicy.h"

typedef struct WorkspaceContainer WorkspaceContainer;
typedef struct DockHostWindow DockHostWindow;

FloatingDockChildHostKind FloatingChildHost_GetKind(HWND hWndChild);
int FloatingChildHost_CollectDocumentWorkspaceHwnds(HWND hWndChild, HWND* pWorkspaceHwnds, int cWorkspaceHwnds);
BOOL FloatingChildHost_GetDocumentSourceContext(HWND hWndChild, POINT ptScreen, WorkspaceContainer** ppWorkspaceSource, int* pnSourceDocumentCount);
BOOL FloatingChildHost_MoveDocumentsToWorkspace(HWND hWndChild, WorkspaceContainer* pWorkspaceTarget);
BOOL FloatingChildHost_EnsureWorkspaceChildForSideDock(HWND hWndFloating, HWND* phWndChild);
DockHostWindow* FloatingChildHost_EnsureTargetWorkspaceDockHost(HWND hWndTargetWorkspace);

#pragma once

#include "win32/window.h"

typedef struct ViewportVector ViewportVector;
typedef struct ViewportWindow ViewportWindow;

typedef struct WorkspaceContainer WorkspaceContainer;
struct WorkspaceContainer {
    Window base;

    ViewportWindow* m_pViewportWindow;
    ViewportVector* m_pViewportVector;
    int m_iPressedTabIndex;
    int m_iPressedCloseTabIndex;
    POINT m_ptTabDragStart;
    void* pFileDropTarget;
};

WorkspaceContainer* WorkspaceContainer_Create();
void WorkspaceContainer_AddViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow);
void WorkspaceContainer_SetCurrentViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, BOOL bSyncAppState);
BOOL WorkspaceContainer_DetachViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow);
void WorkspaceContainer_ClearAllViewports(WorkspaceContainer* pWorkspaceContainer);
ViewportWindow* WorkspaceContainer_GetCurrentViewport(WorkspaceContainer* pWorkspaceContainer);
ViewportWindow* WorkspaceContainer_GetViewportAt(WorkspaceContainer* pWorkspaceContainer, int index);
int WorkspaceContainer_GetViewportCount(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_FloatViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, int xScreen, int yScreen, BOOL bStartMove);
void WorkspaceContainer_MoveAllViewportsTo(WorkspaceContainer* pSourceWorkspace, WorkspaceContainer* pTargetWorkspace);
WorkspaceContainer* WorkspaceContainer_FindDropTargetAtScreenPoint(WorkspaceContainer* pSourceWorkspace, POINT ptScreen);
BOOL WorkspaceContainer_TryDockFloating(WorkspaceContainer* pSourceWorkspace, BOOL bForceMainWorkspace);

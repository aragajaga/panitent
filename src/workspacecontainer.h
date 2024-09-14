#pragma once

#include "win32/window.h"

typedef struct ViewportVector ViewportVector;
typedef struct ViewportWindow ViewportWindow;

typedef struct WorkspaceContainer WorkspaceContainer;
struct WorkspaceContainer {
    Window base;

    ViewportWindow* m_pViewportWindow;
    ViewportVector* m_pViewportVector;
};

WorkspaceContainer* WorkspaceContainer_Create();
void WorkspaceContainer_AddViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow);
ViewportWindow* WorkspaceContainer_GetCurrentViewport(WorkspaceContainer* pWorkspaceContainer);

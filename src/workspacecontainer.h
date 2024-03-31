#pragma once

#include "win32/window.h"

typedef struct ViewportVector ViewportVector;

typedef struct WorkspaceContainer WorkspaceContainer;
struct WorkspaceContainer {
    Window base;

    ViewportWindow* m_pViewportWindow;
    ViewportVector* m_pViewportVector;
};

WorkspaceContainer* WorkspaceContainer_Create(struct Application* app);
void WorkspaceContainer_AddViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow);

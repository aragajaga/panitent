#pragma once

#include "precomp.h"

typedef struct WorkspaceContainer WorkspaceContainer;

BOOL DocumentDockTransition_DockSourceToWorkspace(
    HWND hWndSourceRoot,
    HWND* phWndSourceChild,
    WorkspaceContainer* pWorkspaceTarget,
    int nDockSide,
    int iDockSizeHint);

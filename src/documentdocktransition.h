#pragma once

#include "precomp.h"

typedef struct WorkspaceContainer WorkspaceContainer;
typedef struct DockHostWindow DockHostWindow;
typedef struct DockTargetHit DockTargetHit;

typedef BOOL (*FnDocumentDockTransitionDockTestHook)(
    DockHostWindow* pTargetDockHostWindow,
    HWND hWndChild,
    const DockTargetHit* pDockTarget,
    int iDockSize);

BOOL DocumentDockTransition_DockSourceToWorkspace(
    HWND hWndSourceRoot,
    HWND* phWndSourceChild,
    WorkspaceContainer* pWorkspaceTarget,
    int nDockSide,
    int iDockSizeHint);

void DocumentDockTransition_SetDockTestHook(FnDocumentDockTransitionDockTestHook pfnHook);

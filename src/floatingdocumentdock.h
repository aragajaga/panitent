#pragma once

#include "precomp.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;
typedef struct WorkspaceContainer WorkspaceContainer;

typedef struct WorkspaceDockTargetHit WorkspaceDockTargetHit;
struct WorkspaceDockTargetHit {
    WorkspaceContainer* pWorkspaceTarget;
    int nDockSide;
    RECT rcTargetScreen;
    RECT rcPreviewScreen;
    BOOL bSupportsSplit;
};

BOOL FloatingDocumentDock_GetSourceContext(
    FloatingWindowContainer* pFloatingWindowContainer,
    POINT ptScreen,
    WorkspaceContainer** ppWorkspaceSource,
    int* pnSourceDocumentCount);

BOOL FloatingDocumentDock_HitTestTarget(
    FloatingWindowContainer* pFloatingWindowContainer,
    WorkspaceContainer* pWorkspaceSource,
    int nSourceDocuments,
    POINT ptScreen,
    WorkspaceDockTargetHit* pDockHit);

BOOL FloatingDocumentDock_Attempt(
    FloatingWindowContainer* pFloatingWindowContainer,
    BOOL bForceMainWorkspace);

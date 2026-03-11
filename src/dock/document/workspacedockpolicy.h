#pragma once

#include "precomp.h"

/*
 * Controls whether document-side split targets (left/right/top/bottom) should
 * be available for the current document drag operation.
 */
BOOL WorkspaceDockPolicy_CanSplitTarget(
    int nSourceDocumentCount,
    int nTargetDocumentCount,
    BOOL bTargetSupportsSplitByHost);

/*
 * Empty docked document groups should be removed, except the main workspace.
 */
BOOL WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(
    int nDocumentCount,
    BOOL bIsMainWorkspace,
    BOOL bIsDockedInHost);

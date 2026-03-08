#pragma once

#include "docktypes.h"

typedef enum FloatingDockPolicy
{
	FLOAT_DOCK_POLICY_PANEL = 0,
	FLOAT_DOCK_POLICY_DOCUMENT = 1
} FloatingDockPolicy;

typedef enum FloatingDockChildHostKind
{
	FLOAT_DOCK_CHILD_NONE = 0,
	FLOAT_DOCK_CHILD_TOOL_PANEL,
	FLOAT_DOCK_CHILD_TOOL_HOST,
	FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE,
	FLOAT_DOCK_CHILD_DOCUMENT_HOST
} FloatingDockChildHostKind;

DockPaneKind FloatingDockPolicy_GetPaneKind(FloatingDockPolicy nDockPolicy);
BOOL FloatingDockPolicy_UsesDocumentFlow(FloatingDockPolicy nDockPolicy);
BOOL FloatingDockPolicy_CanShowDockCommand(FloatingDockPolicy nDockPolicy, FloatingDockChildHostKind nChildKind, BOOL bHasDockHostTarget);
BOOL FloatingDockPolicy_CanUseHostDockTarget(FloatingDockPolicy nDockPolicy, FloatingDockChildHostKind nChildKind, BOOL bHasDockHostTarget);
BOOL FloatingDockPolicy_CanMoveDocumentsToWorkspace(FloatingDockChildHostKind nChildKind);
BOOL FloatingDockPolicy_RequiresWorkspaceMergeForSideDock(FloatingDockChildHostKind nChildKind);

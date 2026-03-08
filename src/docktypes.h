#pragma once

#include "precomp.h"

typedef enum DockNodeRole
{
	DOCK_ROLE_UNKNOWN = 0,
	DOCK_ROLE_ROOT,
	DOCK_ROLE_ZONE,
	DOCK_ROLE_SHELL_SPLIT,
	DOCK_ROLE_ZONE_STACK_SPLIT,
	DOCK_ROLE_PANEL_SPLIT,
	DOCK_ROLE_WORKSPACE,
	DOCK_ROLE_PANEL
} DockNodeRole;

typedef enum DockPaneKind
{
	DOCK_PANE_NONE = 0,
	DOCK_PANE_TOOL,
	DOCK_PANE_DOCUMENT
} DockPaneKind;

DockNodeRole DockNodeRole_Resolve(DockNodeRole role, PCWSTR pszName);
BOOL DockNodeRole_IsRoot(DockNodeRole role, PCWSTR pszName);
BOOL DockNodeRole_IsZone(DockNodeRole role, PCWSTR pszName);
BOOL DockNodeRole_IsStructural(DockNodeRole role, PCWSTR pszName);
BOOL DockNodeRole_UsesProportionalGrip(DockNodeRole role, PCWSTR pszName);
BOOL DockNodeRole_IsCorePanel(DockNodeRole role, PCWSTR pszName);

#pragma once

#include "dockhost.h"

typedef enum DockGroupKind
{
	DOCK_GROUP_NONE = 0,
	DOCK_GROUP_TOOL_PANE,
	DOCK_GROUP_DOCUMENT_PANE
} DockGroupKind;

DockGroupKind DockGroup_GetNodeKind(const TreeNode* pNode);
DockPaneKind DockGroup_GetNodePaneKind(const TreeNode* pNode);
BOOL DockGroup_NodeContainsPaneKind(const TreeNode* pNode, DockPaneKind nPaneKind);
TreeNode* DockGroup_FindOwningGroup(TreeNode* pRoot, TreeNode* pNode);
BOOL DockGroup_CanTabIntoGroup(DockGroupKind nGroupKind, DockPaneKind nPaneKind);
BOOL DockGroup_CanSplitGroup(DockGroupKind nGroupKind, DockPaneKind nPaneKind);

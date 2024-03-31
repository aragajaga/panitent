#pragma once

#include "win32/dialog.h"
#include "win32/tree_view.h"

typedef struct TreeNode TreeNode;

typedef struct DockInspectorDialog DockInspectorDialog;
struct DockInspectorDialog {
	Dialog base;
	TreeViewCtl* m_pTreeView;

	TreeNode* m_pTreeRoot;
};

DockInspectorDialog* DockInspectorDialog_Create();
void DockInspectorDialog_Init(DockInspectorDialog* pDockInspectorDialog, Application* pApp);
void DockInspectorDialog_Update(DockInspectorDialog* pDockInspectorDialog, TreeNode* pTreeRoot);

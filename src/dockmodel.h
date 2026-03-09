#pragma once

#include "dockhost.h"

typedef struct DockModelNode DockModelNode;
struct DockModelNode
{
	uint32_t uNodeId;
	uint32_t uViewId;
	DockNodeRole nRole;
	DockPaneKind nPaneKind;
	int nDockSide;
	DWORD dwStyle;
	float fGripPos;
	short iGripPos;
	BOOL bShowCaption;
	BOOL bCollapsed;
	WCHAR szName[MAX_PATH];
	WCHAR szCaption[MAX_PATH];
	WCHAR szActiveTabName[MAX_PATH];
	DockModelNode* pChild1;
	DockModelNode* pChild2;
};

DockModelNode* DockModel_CaptureTree(const TreeNode* pRootNode);
DockModelNode* DockModel_CaptureHostLayout(const DockHostWindow* pDockHostWindow);
void DockModel_Destroy(DockModelNode* pNode);

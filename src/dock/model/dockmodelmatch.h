#pragma once

#include "dockhost.h"
#include "dockmodel.h"

DockModelNode* DockModelMatch_FindByViewId(DockModelNode* pNode, PanitentDockViewId nViewId);
DockModelNode* DockModelMatch_FindByName(DockModelNode* pNode, PCWSTR pszName);
DockModelNode* DockModelMatch_FindWorkspaceByLiveHwnd(const TreeNode* pLiveRoot, DockModelNode* pModelRoot, HWND hWndWorkspace);
DockModelNode* DockModelMatch_FindNodeForLiveData(const TreeNode* pLiveRoot, DockModelNode* pModelRoot, const DockData* pDockData);

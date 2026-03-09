#pragma once

#include "dockhost.h"

BOOL DockHostLayout_NodeHasVisibleWindow(TreeNode* pNode);
void DockHostLayout_AssignRects(TreeNode* pRootNode, int iBorderWidth, int iMinPaneSize);

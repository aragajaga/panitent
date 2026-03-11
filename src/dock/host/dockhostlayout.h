#pragma once

#include "dockhost.h"

BOOL DockHostLayout_NodeHasVisibleWindow(TreeNode* pNode);
BOOL DockHostLayout_IsSplitVertical(TreeNode* pNode);
BOOL DockHostLayout_GetSplitRect(TreeNode* pNode, RECT* pRect, int iBorderWidth);
void DockHostLayout_AssignRects(TreeNode* pRootNode, int iBorderWidth, int iMinPaneSize);

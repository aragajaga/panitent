#pragma once

#include "dockhost.h"

TreeNode* DockNode_FindParent(TreeNode* root, TreeNode* node);
TreeNode* DockNode_FindByHWND(TreeNode* pNode, HWND hWnd);
TreeNode* DockNode_FindZoneBySide(TreeNode* pNode, int nDockSide);
TreeNode* DockHostWindow_GetZoneNode(DockHostWindow* pDockHostWindow, int nDockSide);
int DockHostWindow_GetPanelDockSide(DockHostWindow* pDockHostWindow, HWND hWndPanel);
TreeNode* DockHostWindow_FindOwningZoneNode(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode);

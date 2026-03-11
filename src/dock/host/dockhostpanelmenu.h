#pragma once

#include "dockhost.h"

BOOL DockHostPanelMenu_TogglePanelPinned(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode);
BOOL DockHostPanelMenu_MovePanelToNewWindow(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode);
void DockHostPanelMenu_Show(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode, POINT ptScreen);

#pragma once

#include "dockhost.h"

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y);
BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y);
BOOL Dock_PinButtonHitTest(DockData* pDockData, int x, int y);
BOOL Dock_MoreButtonHitTest(DockData* pDockData, int x, int y);
void DockNode_Paint(DockHostWindow* pDockHostWindow, TreeNode* pNodeParent, HDC hdc, HBRUSH hCaptionBrush);

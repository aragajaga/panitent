#pragma once

#include "dockhost.h"

void DockHostDrag_DestroyOverlay(void);
void DockHostDrag_StartDrag(DockHostWindow* pDockHostWindow, int x, int y);
void DockHostDrag_UpdateOverlayVisual(DockHostWindow* pDockHostWindow, int iRadius);
void DockHostDrag_UndockToFloating(DockHostWindow* pDockHostWindow, TreeNode* pNode, int x, int y);
int DockHostDrag_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen);
BOOL DockHostDrag_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit);

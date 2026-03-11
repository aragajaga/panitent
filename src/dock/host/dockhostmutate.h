#pragma once

#include "dockhost.h"

void DockHostMutate_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostMutate_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
BOOL DockHostMutate_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize);
BOOL DockHostMutate_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize);
BOOL DockHostMutate_RemoveDockedWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive);

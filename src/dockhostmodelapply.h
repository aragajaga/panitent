#pragma once

#include "dockhost.h"

BOOL DockHostModelApply_DockToolWindow(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize);
BOOL DockHostModelApply_RemoveToolWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive);
BOOL DockHostModelApply_DockDocumentWindow(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize);

#pragma once

#include "dockhost.h"

BOOL DockHostModelApply_DockDocumentWindow(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize);
BOOL DockHostModelApply_RemoveDocumentWindow(DockHostWindow* pDockHostWindow, HWND hWnd, BOOL bKeepWindowAlive);
BOOL DockHostModelApply_UndockDocumentWindowToFloating(
    DockHostWindow* pDockHostWindow,
    HWND hWnd,
    const RECT* pFloatingWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut);

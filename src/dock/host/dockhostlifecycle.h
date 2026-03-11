#pragma once

#include "dockhost.h"

BOOL DockHostWindow_OnCreate(DockHostWindow* pDockHostWindow, LPCREATESTRUCT lpcs);
void DockHostWindow_OnDestroy(DockHostWindow* pDockHostWindow);
void DockHostWindow_OnPaint(DockHostWindow* pDockHostWindow);
void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy);
BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

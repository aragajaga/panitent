#pragma once

#include "dockhost.h"

BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd);
DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd);
void DockData_PinHWND(DockHostWindow* pDockHostWindow, DockData* pDockData, HWND hWnd);
void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window);

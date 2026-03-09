#pragma once

#include "dockhost.h"

void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow);
void DockData_PinHWND(DockHostWindow* pDockHostWindow, DockData* pDockData, HWND hWnd);
void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window);
BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect);
void DockHostWindow_ClearLayout(DockHostWindow* pDockHostWindow, const HWND* phWndPreserve, int cPreserve);
void DockHostWindow_DestroyNodeTree(TreeNode* pRootNode, const HWND* phWndPreserve, int cPreserve);
DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd);
BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd);
BOOL DockData_GetClientRect(DockData* pDockData, RECT* rc);
void DockData_Init(DockData* pDockData);
BOOL DockData_GetCaptionRect(DockData* pDockData, RECT* rc);
void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow);

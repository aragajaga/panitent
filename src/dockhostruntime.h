#pragma once

#include "dockhost.h"

BOOL DockHostWindow_OnCreate(DockHostWindow* pDockHostWindow, LPCREATESTRUCT lpcs);
void DockHostWindow_OnDestroy(DockHostWindow* pDockHostWindow);
void DockHostWindow_OnPaint(DockHostWindow* pDockHostWindow);
void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy);
BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow);
DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
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

#pragma once

#include "dockhost.h"

void DockHostWindow_PreRegister(LPWNDCLASSEX lpwcex);
void DockHostWindow_PreCreate(LPCREATESTRUCT lpcs);
DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
void DockHostWindow_Init(DockHostWindow* pDockHostWindow, PanitentApp* pPanitentApp);
DockHostWindow* DockHostWindow_Create(PanitentApp* pPanitentApp);
TreeNode* DockHostWindow_SetRoot(DockHostWindow* pDockHostWindow, TreeNode* pNewRoot);
TreeNode* DockHostWindow_GetRoot(DockHostWindow* pDockHostWindow);

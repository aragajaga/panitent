#pragma once

#include "precomp.h"

#include "util/tree.h"

#define DGA_START 0
#define DGA_END 1

#define DGP_ABSOLUTE 0
#define DGP_RELATIVE 2

#define DGD_HORIZONTAL 0
#define DGD_VERTICAL 4

typedef struct DockData DockData;
struct DockData {
	HWND hWnd;
	DWORD dwStyle;
	WCHAR lpszCaption[MAX_PATH];
	WCHAR lpszName[MAX_PATH];
	RECT rc;
	float fGripPos;
	short iGripPos;
	BOOL bShowCaption;
};

typedef struct DockInspectorDialog DockInspectorDialog;

typedef struct DockHostWindow DockHostWindow;
struct DockHostWindow {
	Window base;

	BOOL fCaptionDrag;
	DockData* m_pSubjectNode;
	HBRUSH hCaptionBrush_;
	TreeNode* pRoot_;
	POINT ptDragPos_;
	BOOL fDrag_;
	DockInspectorDialog* m_pDockInspectorDialog;
};

extern TreeNode* g_pRoot;

typedef struct PanitentApp PanitentApp;

DockHostWindow* DockHostWindow_Create(PanitentApp* app);
void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window);
DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockHostWindow_SetRoot(DockHostWindow* pDockHostWindow, TreeNode* pNewRoot);
TreeNode* DockHostWindow_GetRoot(DockHostWindow* pDockHostWindow);

#define DKS_NONE 0
#define DKS_LEFT 1
#define DKS_RIGHT 2
#define DKS_TOP 3
#define DKS_BOTTOM 4

int DockHostWindow_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen);
BOOL DockHostWindow_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize);

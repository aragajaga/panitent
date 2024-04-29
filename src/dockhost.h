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

DockHostWindow* DockHostWindow_Create(struct Application* app);
void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window);
DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);

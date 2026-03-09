#pragma once

#include "precomp.h"

#include "win32/window.h"
#include "util/tree.h"
#include "docktypes.h"
#include "dockviewcatalog.h"

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
	uint32_t uModelNodeId;
	PanitentDockViewId nViewId;
	DockNodeRole nRole;
	DockPaneKind nPaneKind;
	int nDockSide;
	RECT rc;
	float fGripPos;
	short iGripPos;
	BOOL bShowCaption;
	BOOL bCollapsed;
	HWND hWndActiveTab;
};

typedef struct DockInspectorDialog DockInspectorDialog;

typedef struct DockHostWindow DockHostWindow;
struct DockHostWindow {
	Window base;

	BOOL fCaptionDrag;
	TreeNode* m_pSubjectNode;
	BOOL fSplitDrag;
	TreeNode* pSplitNode;
	POINT ptSplitDragStart;
	int iSplitDragStartGrip;
	HBRUSH hCaptionBrush_;
	TreeNode* pRoot_;
	POINT ptDragPos_;
	BOOL fDrag_;
	BOOL fAutoHideOverlayVisible;
	int nAutoHideOverlaySide;
	HWND hWndAutoHideOverlay;
	HWND hWndAutoHideOverlayHost;
	RECT rcAutoHideOverlay;
	TreeNode* pCaptionHotNode;
	TreeNode* pCaptionPressedNode;
	int nCaptionHotButton;
	int nCaptionPressedButton;
	BOOL fAutoHideOverlayTrackMouse;
	int nAutoHideOverlayHotButton;
	int nAutoHideOverlayPressedButton;
	DockInspectorDialog* m_pDockInspectorDialog;
	void* pFileDropTarget;
};

extern TreeNode* g_pRoot;

typedef struct PanitentApp PanitentApp;

DockHostWindow* DockHostWindow_Create(PanitentApp* app);
void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow);
void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window);
DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption);
TreeNode* DockHostWindow_SetRoot(DockHostWindow* pDockHostWindow, TreeNode* pNewRoot);
TreeNode* DockHostWindow_GetRoot(DockHostWindow* pDockHostWindow);
void DockHostWindow_ClearLayout(DockHostWindow* pDockHostWindow, const HWND* phWndPreserve, int cPreserve);
void DockHostWindow_DestroyNodeTree(TreeNode* pRootNode, const HWND* phWndPreserve, int cPreserve);

#define DKS_NONE 0
#define DKS_LEFT 1
#define DKS_RIGHT 2
#define DKS_TOP 3
#define DKS_BOTTOM 4
#define DKS_CENTER 5

typedef struct DockTargetHit DockTargetHit;
struct DockTargetHit {
	int nDockSide;
	BOOL bLocalTarget;
	HWND hWndAnchor;
	RECT rcAnchorClient;
	RECT rcPreviewClient;
};

int DockHostWindow_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen);
BOOL DockHostWindow_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit);
BOOL DockHostWindow_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize);
BOOL DockHostWindow_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize);
BOOL DockHostWindow_DestroyDockedHWND(DockHostWindow* pDockHostWindow, HWND hWnd);
void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow);

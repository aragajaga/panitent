#include "precomp.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "dockhost.h"
#include "dockhostdrag.h"
#include "dockhostinput.h"
#include "dockhostmetrics.h"
#include "dockhostmutate.h"
#include "dockhostruntime.h"
#include "dockhostzone.h"
#include "dockinspectordialog.h"
#include "resource.h"

#include "panitentapp.h"

static const WCHAR szClassName[] = L"__DockHostWindow";

#define IDM_DOCKINSPECTOR 101

void Dock_DestroyInclusive(TreeNode*, TreeNode*);
BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow);

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow);

void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	DockHostMutate_DestroyInclusive(pDockHostWindow, pTargetNode);
}

void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	DockHostMutate_Undock(pDockHostWindow, pTargetNode);
}

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDM_DOCKINSPECTOR:
		DockHostInput_InvokeInspectorDialog(pDockHostWindow);
		return TRUE;
	}

	return FALSE;
}

LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ERASEBKGND:
		/* Full background is painted in OnPaint using a backbuffer. */
		return 1;
		break;

	case WM_MOUSEMOVE:
		DockHostInput_OnMouseMove(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam, DockHostMetrics_GetZoneTabGutter());
		return 0;
		break;

	case WM_MOUSELEAVE:
		DockHostInput_OnMouseLeave(pDockHostWindow);
		return 0;
		break;

	case WM_LBUTTONDOWN:
		DockHostInput_OnLButtonDown(pDockHostWindow, FALSE, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam, DockHostMetrics_GetZoneTabGutter());
		return 0;
		break;

	case WM_LBUTTONUP:
		DockHostInput_OnLButtonUp(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_CAPTURECHANGED:
			DockHostInput_OnCaptureChanged(pDockHostWindow);
			return 0;
			break;

	case WM_CONTEXTMENU:
			DockHostInput_OnContextMenu(pDockHostWindow, (HWND)wParam, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam));
			return 0;
			break;
	}

	return Window_UserProcDefault((Window *)pDockHostWindow, hWnd, message, wParam, lParam);
}

void DockHostWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
	lpwcex->style = CS_VREDRAW | CS_HREDRAW;
	lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
	lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	lpwcex->lpszClassName = szClassName;
}

void DockHostWindow_PreCreate(LPCREATESTRUCT lpcs)
{
	lpcs->dwExStyle = 0;
	lpcs->lpszClass = szClassName;
	lpcs->lpszName = L"DockHost";
	lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN;
	lpcs->x = 0;
	lpcs->y = 0;
	lpcs->cx = 0;
	lpcs->cy = 0;
}

void DockHostWindow_Init(DockHostWindow* pDockHostWindow, PanitentApp* pPanitentApp)
{
	Window_Init(&pDockHostWindow->base);

	pDockHostWindow->base.szClassName = szClassName;

	pDockHostWindow->base.OnCreate = (FnWindowOnCreate)DockHostWindow_OnCreate;
	pDockHostWindow->base.OnDestroy = (FnWindowOnDestroy)DockHostWindow_OnDestroy;
	pDockHostWindow->base.OnPaint = (FnWindowOnPaint)DockHostWindow_OnPaint;
	pDockHostWindow->base.OnSize = (FnWindowOnSize)DockHostWindow_OnSize;
	pDockHostWindow->base.OnCommand = (FnWindowOnCommand)DockHostWindow_OnCommand;

	_WindowInitHelper_SetPreRegisterRoutine((Window *)pDockHostWindow, (FnWindowPreRegister)DockHostWindow_PreRegister);
	_WindowInitHelper_SetPreCreateRoutine((Window *)pDockHostWindow, (FnWindowPreCreate)DockHostWindow_PreCreate);
	_WindowInitHelper_SetUserProcRoutine((Window *)pDockHostWindow, (FnWindowUserProc)DockHostWindow_UserProc);

	pDockHostWindow->pRoot_ = NULL;
	pDockHostWindow->fSplitDrag = FALSE;
	pDockHostWindow->pSplitNode = NULL;
	pDockHostWindow->iSplitDragStartGrip = 0;
	pDockHostWindow->fAutoHideOverlayVisible = FALSE;
	pDockHostWindow->nAutoHideOverlaySide = DKS_NONE;
	pDockHostWindow->hWndAutoHideOverlay = NULL;
	SetRectEmpty(&pDockHostWindow->rcAutoHideOverlay);
	pDockHostWindow->pCaptionHotNode = NULL;
	pDockHostWindow->pCaptionPressedNode = NULL;
	pDockHostWindow->nCaptionHotButton = DCB_NONE;
	pDockHostWindow->nCaptionPressedButton = DCB_NONE;
	pDockHostWindow->fAutoHideOverlayTrackMouse = FALSE;
	pDockHostWindow->nAutoHideOverlayHotButton = DCB_NONE;
	pDockHostWindow->nAutoHideOverlayPressedButton = DCB_NONE;

	pDockHostWindow->m_pDockInspectorDialog = DockInspectorDialog_Create();
}

DockHostWindow* DockHostWindow_Create(PanitentApp* pPanitentApp)
{
	DockHostWindow* pDockHostWindow = (DockHostWindow*)malloc(sizeof(DockHostWindow));

	if (pDockHostWindow)
	{
		memset(pDockHostWindow, 0, sizeof(DockHostWindow));
		DockHostWindow_Init(pDockHostWindow, pPanitentApp);
	}

	return pDockHostWindow;
}

TreeNode* DockHostWindow_SetRoot(DockHostWindow* pDockHostWindow, TreeNode* pNewRoot)
{
	TreeNode* pOldRoot = pDockHostWindow->pRoot_;
	pDockHostWindow->pRoot_ = pNewRoot;
	return pOldRoot;
}

TreeNode* DockHostWindow_GetRoot(DockHostWindow* pDockHostWindow)
{
	return pDockHostWindow->pRoot_;
}

int DockHostWindow_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen)
{
	return DockHostDrag_HitTestDockSide(pDockHostWindow, ptScreen);
}

BOOL DockHostWindow_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit)
{
	return DockHostDrag_HitTestDockTarget(pDockHostWindow, ptScreen, pTargetHit);
}

BOOL DockHostWindow_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize)
{
	return DockHostMutate_DockHWND(pDockHostWindow, hWnd, nDockSide, iDockSize);
}

BOOL DockHostWindow_DockHWNDToTarget(DockHostWindow* pDockHostWindow, HWND hWnd, const DockTargetHit* pTargetHit, int iDockSize)
{
	return DockHostMutate_DockHWNDToTarget(pDockHostWindow, hWnd, pTargetHit, iDockSize);
}

BOOL DockHostWindow_DestroyDockedHWND(DockHostWindow* pDockHostWindow, HWND hWnd)
{
	if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
	{
		return FALSE;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	if (!pRoot)
	{
		return FALSE;
	}

	TreeNode* pNode = DockNode_FindByHWND(pRoot, hWnd);
	if (!pNode)
	{
		return FALSE;
	}

	DockHostWindow_DestroyInclusive(pDockHostWindow, pNode);
	return TRUE;
}

DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
	DockData* pDockData = (DockData*)malloc(sizeof(DockData));

	if (pDockData)
	{
		memset(pDockData, 0, sizeof(DockData));

		pDockData->dwStyle = dwStyle;
		pDockData->fGripPos = -1.0f;
		pDockData->iGripPos = iGripPos;
		pDockData->bShowCaption = bShowCaption;
		pDockData->bCollapsed = FALSE;
		pDockData->hWndActiveTab = NULL;
		pDockData->nRole = DOCK_ROLE_UNKNOWN;
		pDockData->nPaneKind = DOCK_PANE_NONE;
		pDockData->nDockSide = DKS_NONE;
		pDockData->uModelNodeId = 0;
		pDockData->nViewId = PNT_DOCK_VIEW_NONE;

		return pDockData;
	}

	return NULL;
}

TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
	TreeNode* pTreeNode = BinaryTree_AllocEmptyNode();
	DockData* pDockData = DockData_Create(iGripPos, dwStyle, bShowCaption);

	pTreeNode->data = pDockData;

	return pTreeNode;
}

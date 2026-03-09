#include "precomp.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "win32/dialog.h"
#include "win32/util.h"

#include <windowsx.h>
#include <commctrl.h>
#include <strsafe.h>
#include <assert.h>

#include "util/stack.h"
#include "util/tree.h"
#include "dockhost.h"
#include "dockhostcaption.h"
#include "dockhostautohide.h"
#include "dockhostdrag.h"
#include "dockhostinput.h"
#include "dockhostmodelapply.h"
#include "dockhostmutate.h"
#include "dockhostpanelmenu.h"
#include "dockhostpaint.h"
#include "dockhostruntime.h"
#include "dockhosttree.h"
#include "dockhostzone.h"
#include "dockgroup.h"
#include "dockhostlayout.h"
#include "docklayout.h"
#include "dockpolicy.h"
#include "dockviewcatalog.h"
#include "resource.h"
#include "floatingwindowcontainer.h"
#include "dockinspectordialog.h"
#include "oledroptarget.h"
#include "toolwndframe.h"
#include "theme.h"

#include "panitentapp.h"

static const WCHAR szClassName[] = L"__DockHostWindow";

POINT captionPos;

BOOL fSuggestTop;

int iCaptionHeight = 14;
int iBorderWidth = 4;
int iZoneTabGutter = 24;
static int g_iZoneTabGutterLeft = 0;
static int g_iZoneTabGutterRight = 0;
static int g_iZoneTabGutterTop = 0;
static int g_iZoneTabGutterBottom = 0;

#define DOCK_ZONE_MAX_TABS 32
#define DOCK_CAPTION_INSET 3
#define DRAG_UNDOCK_DISTANCE 32
#define IDM_DOCKINSPECTOR 101
#define WINDOWBUTTONSIZE 14
#define WINDOWBUTTONSPACING 3
#define DOCK_MIN_PANE_SIZE 96
static BOOL DockHostWindow_OnDropFiles(void* pContext, HDROP hDrop);

void Dock_DestroyInclusive(TreeNode*, TreeNode*);
static void DockHostWindow_SyncZones(DockHostWindow* pDockHostWindow);
BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow);

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode);
void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow);

BOOL DockData_GetClientRect(DockData* pDockData, RECT* rc)
{
	if (!pDockData)
	{
		return FALSE;
	}

	RECT rcClient = pDockData->rc;

	if (pDockData->bShowCaption)
	{
		rcClient.top += iCaptionHeight + 1;
	}

	if (DockNodeRole_IsRoot(pDockData->nRole, pDockData->lpszName))
	{
		rcClient.left += g_iZoneTabGutterLeft;
		rcClient.right -= g_iZoneTabGutterRight;
		rcClient.top += g_iZoneTabGutterTop;
		rcClient.bottom -= g_iZoneTabGutterBottom;
		if (rcClient.left > rcClient.right)
		{
			rcClient.left = rcClient.right;
		}
		if (rcClient.top > rcClient.bottom)
		{
			rcClient.top = rcClient.bottom;
		}
	}

	/* TODO: memcpy? */
	*rc = rcClient;

	return TRUE;
}

void DockData_Init(DockData* pDockData);

void DockData_Init(DockData* pDockData)
{
	pDockData->iGripPos = 64;
	pDockData->dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
	pDockData->bShowCaption = FALSE;
	pDockData->bCollapsed = FALSE;
	pDockData->hWndActiveTab = NULL;
	pDockData->nRole = DOCK_ROLE_UNKNOWN;
	pDockData->nPaneKind = DOCK_PANE_NONE;
	pDockData->nDockSide = DKS_NONE;
	pDockData->uModelNodeId = 0;
	pDockData->nViewId = PNT_DOCK_VIEW_NONE;
}

BOOL DockData_GetCaptionRect(DockData* pDockData, RECT* rc)
{
	if (!pDockData || !rc || !pDockData->bShowCaption)
	{
		return FALSE;
	}

	CaptionFrameMetrics metrics = { 0 };
	metrics.borderSize = DOCK_CAPTION_INSET;
	metrics.captionHeight = iCaptionHeight;
	metrics.buttonSpacing = WINDOWBUTTONSPACING;
	metrics.textPaddingLeft = DOCK_CAPTION_INSET;
	metrics.textPaddingRight = DOCK_CAPTION_INSET;
	metrics.textPaddingY = 0;
	CaptionFrameLayout layout = { 0 };
	if (!CaptionFrame_BuildLayout(&pDockData->rc, &metrics, NULL, 0, &layout))
	{
		return FALSE;
	}

	*rc = layout.rcCaption;
	return TRUE;
}

#define DHT_UNKNOWN 0
#define DHT_CAPTION 1
#define DHT_CLOSEBTN 2
#define DHT_PINBTN 3
#define DHT_MOREBTN 4

static void DockHostWindow_SyncZones(DockHostWindow* pDockHostWindow)
{
	DockHostZone_Sync(
		pDockHostWindow,
		iZoneTabGutter,
		&g_iZoneTabGutterLeft,
		&g_iZoneTabGutterRight,
		&g_iZoneTabGutterTop,
		&g_iZoneTabGutterBottom);
}

void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow)
{
	TreeNode* pRoot = pDockHostWindow->pRoot_;

	if (!pRoot)
	{
		return;
	}

	DockHostWindow_SyncZones(pDockHostWindow);
	DockHostLayout_AssignRects(pRoot, iBorderWidth, DOCK_MIN_PANE_SIZE);

	PostOrderTreeTraversal traversal = { 0 };
	PostOrderTreeTraversal_Init(&traversal, pRoot);
	TreeNode* pCurrentNode = NULL;
	while (pCurrentNode = PostOrderTreeTraversal_GetNext(&traversal))
	{
		DockData* pDockData = (DockData*)pCurrentNode->data;
			if (pDockData && pDockData->hWnd)
			{
				if (pDockHostWindow->fAutoHideOverlayVisible &&
					pDockHostWindow->hWndAutoHideOverlayHost &&
					IsWindow(pDockHostWindow->hWndAutoHideOverlayHost) &&
					pDockData->hWnd == pDockHostWindow->hWndAutoHideOverlay &&
					GetParent(pDockData->hWnd) == pDockHostWindow->hWndAutoHideOverlayHost)
				{
					continue;
				}

			RECT rc = { 0 };
			DockData_GetClientRect(pDockData, &rc);
			if (!DockHostWindow_IsWorkspaceWindow(pDockData->hWnd))
			{
				Win32_ContractRect(&rc, 4, 4);
			}
			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;

			if (width <= 12 || height <= 12)
			{
				ShowWindow(pDockData->hWnd, SW_HIDE);
			}
			else {
				ShowWindow(pDockData->hWnd, SW_SHOWNA);
				SetWindowPos(pDockData->hWnd, NULL, rc.left, rc.top, width, height,
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			}
		}
	}

	PostOrderTreeTraversal_Destroy(&traversal);

	DockHostWindow_UpdateAutoHideOverlay(pDockHostWindow);
	Window_Invalidate((Window *)pDockHostWindow);

	DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, pDockHostWindow->pRoot_);
}

void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	DockHostMutate_DestroyInclusive(pDockHostWindow, pTargetNode);
}

void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	DockHostMutate_Undock(pDockHostWindow, pTargetNode);
}

BOOL DockHostWindow_OnCreate(DockHostWindow* pDockHostWindow, LPCREATESTRUCT lpcs)
{
	UNREFERENCED_PARAMETER(lpcs);
	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);

	DockHostWindow_RefreshTheme(pDockHostWindow);
	OleFileDropTarget_Register(
		hWnd,
		DockHostWindow_OnDropFiles,
		pDockHostWindow,
		(IDropTarget**)&pDockHostWindow->pFileDropTarget);

	return TRUE;
}

void DockHostWindow_OnDestroy(DockHostWindow* pDockHostWindow)
{
	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);

	if (pDockHostWindow->pFileDropTarget)
	{
		OleFileDropTarget_Revoke(hWnd, (IDropTarget**)&pDockHostWindow->pFileDropTarget);
	}
	DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
	if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
	{
		DestroyWindow(pDockHostWindow->hWndAutoHideOverlayHost);
	}
	pDockHostWindow->hWndAutoHideOverlayHost = NULL;
	if (pDockHostWindow->hCaptionBrush_)
	{
		DeleteObject(pDockHostWindow->hCaptionBrush_);
		pDockHostWindow->hCaptionBrush_ = NULL;
	}
}

static BOOL DockHostWindow_OnDropFiles(void* pContext, HDROP hDrop)
{
	UNREFERENCED_PARAMETER(pContext);
	return PanitentApp_OpenDroppedFiles(PanitentApp_Instance(), hDrop);
}

void DockHostWindow_OnPaint(DockHostWindow* pDockHostWindow)
{
	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
	PAINTSTRUCT ps = { 0 };
	HDC hdc = BeginPaint(hWnd, &ps);

	RECT rcClient = { 0 };
	GetClientRect(hWnd, &rcClient);
	int width = Win32_Rect_GetWidth(&rcClient);
	int height = Win32_Rect_GetHeight(&rcClient);

	HDC hdcTarget = hdc;
	HDC hdcBuffer = NULL;
	HBITMAP hbmBuffer = NULL;
	HGDIOBJ hOldBitmap = NULL;
	if (width > 0 && height > 0)
	{
		hdcBuffer = CreateCompatibleDC(hdc);
		if (hdcBuffer)
		{
			hbmBuffer = CreateCompatibleBitmap(hdc, width, height);
			if (hbmBuffer)
			{
				hOldBitmap = SelectObject(hdcBuffer, hbmBuffer);
				hdcTarget = hdcBuffer;
			}
		}
	}

	DockHostWindow_SyncZones(pDockHostWindow);
	DockHostPaint_PaintContent(
		pDockHostWindow,
		hdcTarget,
		&rcClient,
		pDockHostWindow->hCaptionBrush_,
		iZoneTabGutter,
		iBorderWidth);

	if (hdcTarget == hdcBuffer)
	{
		BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);
		SelectObject(hdcBuffer, hOldBitmap);
		DeleteObject(hbmBuffer);
		DeleteDC(hdcBuffer);
	}
	else if (hdcBuffer)
	{
		DeleteDC(hdcBuffer);
	}

	EndPaint(hWnd, &ps);
}

void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy)
{
	UNREFERENCED_PARAMETER(state);

	if (pDockHostWindow->pRoot_ && pDockHostWindow->pRoot_->data)
	{
		RECT rcRoot = { 0 };
		rcRoot.right = cx;
		rcRoot.bottom = cy;
		((DockData*)pDockHostWindow->pRoot_->data)->rc = rcRoot;

		DockHostWindow_Rearrange(pDockHostWindow);
	}
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
		DockHostInput_OnMouseMove(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam, iZoneTabGutter);
		return 0;
		break;

	case WM_MOUSELEAVE:
		DockHostInput_OnMouseLeave(pDockHostWindow);
		return 0;
		break;

	case WM_LBUTTONDOWN:
		DockHostInput_OnLButtonDown(pDockHostWindow, FALSE, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam, iZoneTabGutter);
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

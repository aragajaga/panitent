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
static void DockHostWindow_AssignPersistentNameForHWND(DockData* pDockData, HWND hWnd);
static BOOL DockNode_IsStructural(TreeNode* pNode);
static BOOL DockNode_UsesProportionalGrip(TreeNode* pNode);
static BOOL DockHostWindow_DockIntoZone(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, HWND hWnd, int nDockSide, int iDockSize);
static void DockHostWindow_UpdateZoneSplitGrip(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, int nDockSide, int iDockSize);
static BOOL DockHostWindow_ToggleZoneCollapsed(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab);
static void DockHostWindow_SyncZones(DockHostWindow* pDockHostWindow);
BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect);
BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow);
static TreeNode* DockHostWindow_HitTestSplitGrip(DockHostWindow* pDockHostWindow, int x, int y);
static int DockHostWindow_HitTestSideInRect(const RECT* pRect, POINT pt, int iThresholdMin, int iThresholdMax);
static BOOL DockHostWindow_DockAroundPanel(DockHostWindow* pDockHostWindow, TreeNode* pAnchorNode, HWND hWnd, int nDockSide, int iDockSize);
static void DockHostWindow_BeginSplitDrag(DockHostWindow* pDockHostWindow, TreeNode* pSplitNode, int x, int y);
static void DockHostWindow_UpdateSplitDrag(DockHostWindow* pDockHostWindow, int x, int y);
static void DockHostWindow_EndSplitDrag(DockHostWindow* pDockHostWindow);
static BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd);
DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd);

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

static BOOL DockNode_IsStructural(TreeNode* pNode)
{
	if (!pNode || !pNode->data)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	return DockNodeRole_IsStructural(pDockData->nRole, pDockData->lpszName);
}

static BOOL DockNode_UsesProportionalGrip(TreeNode* pNode)
{
	if (!pNode || !pNode->data)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	return DockNodeRole_UsesProportionalGrip(pDockData->nRole, pDockData->lpszName);
}

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

BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect)
{
	if (!pDockHostWindow || !pRect)
	{
		return FALSE;
	}

	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
	if (!hWnd || !IsWindow(hWnd))
	{
		return FALSE;
	}

	GetClientRect(hWnd, pRect);
	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	if (pRoot && pRoot->data)
	{
		DockData* pRootData = (DockData*)pRoot->data;
		RECT rcRootClient = { 0 };
		if (DockData_GetClientRect(pRootData, &rcRootClient))
		{
			*pRect = rcRootClient;
		}
	}

	return Win32_Rect_GetWidth(pRect) > 0 && Win32_Rect_GetHeight(pRect) > 0;
}

static BOOL DockHostWindow_ToggleZoneCollapsed(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab)
{
	if (!pDockHostWindow || !hWndTab || !IsWindow(hWndTab))
	{
		return FALSE;
	}

	if (pDockHostWindow->fAutoHideOverlayVisible &&
		pDockHostWindow->nAutoHideOverlaySide == nDockSide &&
		pDockHostWindow->hWndAutoHideOverlay == hWndTab)
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return TRUE;
	}

	return DockHostWindow_ShowAutoHideOverlay(pDockHostWindow, nDockSide, hWndTab);
}

static TreeNode* DockHostWindow_HitTestSplitGrip(DockHostWindow* pDockHostWindow, int x, int y)
{
	if (!pDockHostWindow || !pDockHostWindow->pRoot_)
	{
		return NULL;
	}

	DockHostWindow_SyncZones(pDockHostWindow);

	POINT pt = { x, y };
	TreeTraversalRLOT traversal = { 0 };
	TreeTraversalRLOT_Init(&traversal, pDockHostWindow->pRoot_);

	TreeNode* pResult = NULL;
	TreeNode* pCurrentNode = NULL;
	while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
	{
		RECT rcSplit = { 0 };
		if (!DockHostLayout_GetSplitRect(pCurrentNode, &rcSplit, iBorderWidth))
		{
			continue;
		}

		InflateRect(&rcSplit, 3, 3);
		if (PtInRect(&rcSplit, pt))
		{
			pResult = pCurrentNode;
			break;
		}
	}

	TreeTraversalRLOT_Destroy(&traversal);
	return pResult;
}

static void DockHostWindow_BeginSplitDrag(DockHostWindow* pDockHostWindow, TreeNode* pSplitNode, int x, int y)
{
	if (!pDockHostWindow || !pSplitNode || !pSplitNode->data)
	{
		return;
	}

	pDockHostWindow->fSplitDrag = TRUE;
	pDockHostWindow->pSplitNode = pSplitNode;
	pDockHostWindow->ptSplitDragStart.x = x;
	pDockHostWindow->ptSplitDragStart.y = y;
	pDockHostWindow->iSplitDragStartGrip = ((DockData*)pSplitNode->data)->iGripPos;

	SetCapture(Window_GetHWND((Window*)pDockHostWindow));
}

static void DockHostWindow_UpdateSplitDrag(DockHostWindow* pDockHostWindow, int x, int y)
{
	if (!pDockHostWindow || !pDockHostWindow->fSplitDrag || !pDockHostWindow->pSplitNode || !pDockHostWindow->pSplitNode->data)
	{
		return;
	}

	DockData* pDockData = (DockData*)pDockHostWindow->pSplitNode->data;
	if (pDockData->dwStyle & DGP_RELATIVE)
	{
		return;
	}

	RECT rcClient = { 0 };
	if (!DockData_GetClientRect(pDockData, &rcClient))
	{
		return;
	}

	BOOL bVertical = DockHostLayout_IsSplitVertical(pDockHostWindow->pSplitNode);
	int iSpan = bVertical ? Win32_Rect_GetHeight(&rcClient) : Win32_Rect_GetWidth(&rcClient);
	if (iSpan <= 0)
	{
		return;
	}

	int iDelta = bVertical ? (y - pDockHostWindow->ptSplitDragStart.y) : (x - pDockHostWindow->ptSplitDragStart.x);
	int iNextGrip = DockLayout_AdjustSplitGripFromDelta(pDockData->dwStyle, pDockHostWindow->iSplitDragStartGrip, iDelta, iSpan, DOCK_MIN_PANE_SIZE);
	if (iNextGrip == pDockData->iGripPos)
	{
		return;
	}

	pDockData->iGripPos = (short)iNextGrip;
	if (DockNode_UsesProportionalGrip(pDockHostWindow->pSplitNode))
	{
		pDockData->fGripPos = DockLayout_GetGripRatio(iSpan, iNextGrip, DOCK_MIN_PANE_SIZE);
	}
	DockHostWindow_Rearrange(pDockHostWindow);
}

static void DockHostWindow_EndSplitDrag(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow || !pDockHostWindow->fSplitDrag)
	{
		return;
	}

	pDockHostWindow->fSplitDrag = FALSE;
	pDockHostWindow->pSplitNode = NULL;
	pDockHostWindow->iSplitDragStartGrip = 0;
	ReleaseCapture();
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

void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow)
{
	PanitentThemeColors colors = { 0 };
	HWND hWnd = NULL;

	if (!pDockHostWindow)
	{
		return;
	}

	PanitentTheme_GetColors(&colors);
	if (pDockHostWindow->hCaptionBrush_)
	{
		DeleteObject(pDockHostWindow->hCaptionBrush_);
	}
	pDockHostWindow->hCaptionBrush_ = CreateSolidBrush(colors.accent);

	hWnd = Window_GetHWND((Window*)pDockHostWindow);
	if (hWnd && IsWindow(hWnd))
	{
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}
	if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
	{
		RedrawWindow(
			pDockHostWindow->hWndAutoHideOverlayHost,
			NULL,
			NULL,
			RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}
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

static BOOL DockHostWindow_ShouldPreserveHwnd(HWND hWnd, const HWND* phWndPreserve, int cPreserve)
{
	if (!hWnd || !phWndPreserve || cPreserve <= 0)
	{
		return FALSE;
	}

	for (int i = 0; i < cPreserve; ++i)
	{
		if (phWndPreserve[i] == hWnd)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static void DockHostWindow_DestroyTreeRecursive(TreeNode* pNode, const HWND* phWndPreserve, int cPreserve)
{
	if (!pNode)
	{
		return;
	}

	DockHostWindow_DestroyTreeRecursive(pNode->node1, phWndPreserve, cPreserve);
	DockHostWindow_DestroyTreeRecursive(pNode->node2, phWndPreserve, cPreserve);

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData && pDockData->hWnd && IsWindow(pDockData->hWnd) &&
		!DockHostWindow_ShouldPreserveHwnd(pDockData->hWnd, phWndPreserve, cPreserve))
	{
		DestroyWindow(pDockData->hWnd);
	}

	free(pNode->data);
	free(pNode);
}

void DockHostWindow_DestroyNodeTree(TreeNode* pRootNode, const HWND* phWndPreserve, int cPreserve)
{
	DockHostWindow_DestroyTreeRecursive(pRootNode, phWndPreserve, cPreserve);
}

void DockHostWindow_ClearLayout(DockHostWindow* pDockHostWindow, const HWND* phWndPreserve, int cPreserve)
{
	if (!pDockHostWindow)
	{
		return;
	}

	DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
	DockHostInput_ClearCaptionState(pDockHostWindow);
	DockHostWindow_ClearAutoHideCaptionState(pDockHostWindow);
	DockHostWindow_DestroyNodeTree(pDockHostWindow->pRoot_, phWndPreserve, cPreserve);
	pDockHostWindow->pRoot_ = NULL;
	if (pDockHostWindow->m_pDockInspectorDialog)
	{
		DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, NULL);
	}
	Window_Invalidate((Window*)pDockHostWindow);
}

static BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd)
{
	if (!hWnd || !IsWindow(hWnd))
	{
		return FALSE;
	}

	WCHAR szClassNameBuf[64] = L"";
	GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
	return wcscmp(szClassNameBuf, L"__WorkspaceContainer") == 0;
}

DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd)
{
	WCHAR szClassNameBuf[64] = L"";
	if (!hWnd || !IsWindow(hWnd))
	{
		return DOCK_PANE_NONE;
	}

	if (DockHostWindow_IsWorkspaceWindow(hWnd))
	{
		return DOCK_PANE_DOCUMENT;
	}

	GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
	if (wcscmp(szClassNameBuf, L"__DockHostWindow") == 0)
	{
		Window* pWindow = WindowMap_Get(hWnd);
		DockHostWindow* pDockHostWindow = (DockHostWindow*)pWindow;
		TreeNode* pRoot = pDockHostWindow ? DockHostWindow_GetRoot(pDockHostWindow) : NULL;
		if (DockGroup_NodeContainsPaneKind(pRoot, DOCK_PANE_DOCUMENT))
		{
			return DOCK_PANE_DOCUMENT;
		}
	}

	return DOCK_PANE_TOOL;
}

void DockData_PinHWND(DockHostWindow* pDockHostWindow, DockData* pDockData, HWND hWnd)
{
	if (!pDockHostWindow || !pDockData || !hWnd || !IsWindow(hWnd))
	{
		return;
	}

	GetWindowText(hWnd, pDockData->lpszCaption, MAX_PATH);
	SetParent(hWnd, Window_GetHWND((Window*)pDockHostWindow));

	DWORD dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_POPUP);
	dwStyle |= WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);

	pDockData->bShowCaption = !DockHostWindow_IsWorkspaceWindow(hWnd);
	pDockData->hWnd = hWnd;
	if (DockHostWindow_IsWorkspaceWindow(hWnd))
	{
		pDockData->nRole = DOCK_ROLE_WORKSPACE;
		pDockData->nPaneKind = DOCK_PANE_DOCUMENT;
	}
	else if (pDockData->nRole == DOCK_ROLE_UNKNOWN)
	{
		pDockData->nRole = DOCK_ROLE_PANEL;
		pDockData->nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
	}
	else if (pDockData->nPaneKind == DOCK_PANE_NONE)
	{
		pDockData->nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
	}

	if (pDockData->lpszName[0] == L'\0')
	{
		DockHostWindow_AssignPersistentNameForHWND(pDockData, hWnd);
	}
}

static void DockHostWindow_AssignPersistentNameForHWND(DockData* pDockData, HWND hWnd)
{
	WCHAR szClassNameBuf[64] = L"";
	WCHAR szTitleBuf[MAX_PATH] = L"";
	PanitentDockViewId nViewId;
	PCWSTR pszCanonicalName;

	if (!pDockData || !hWnd || !IsWindow(hWnd))
	{
		return;
	}

	GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
	GetWindowTextW(hWnd, szTitleBuf, ARRAYSIZE(szTitleBuf));
	nViewId = PanitentDockViewCatalog_FindForWindow(szClassNameBuf, szTitleBuf);
	pszCanonicalName = PanitentDockViewCatalog_GetCanonicalName(nViewId);
	if (pszCanonicalName && pszCanonicalName[0] != L'\0')
	{
		pDockData->nViewId = nViewId;
		wcscpy_s(pDockData->lpszName, MAX_PATH, pszCanonicalName);
		return;
	}

	if (pDockData->lpszCaption[0] != L'\0')
	{
		wcscpy_s(pDockData->lpszName, MAX_PATH, pDockData->lpszCaption);
	}
}

static void DockHostWindow_UpdateZoneSplitGrip(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, int nDockSide, int iDockSize)
{
	if (!pDockHostWindow || !pZoneNode)
	{
		return;
	}

	TreeNode* pParent = DockNode_FindParent(DockHostWindow_GetRoot(pDockHostWindow), pZoneNode);
	if (!pParent || !pParent->data)
	{
		return;
	}

	DockData* pDockDataParent = (DockData*)pParent->data;
	pDockDataParent->iGripPos = DockLayout_GetZoneSplitGrip(nDockSide, iDockSize);
}

static BOOL DockHostWindow_DockIntoZone(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, HWND hWnd, int nDockSide, int iDockSize)
{
	if (!pDockHostWindow || !pZoneNode || !hWnd || !IsWindow(hWnd))
	{
		return FALSE;
	}

	DockData* pZoneData = NULL;
	if (pZoneNode->data)
	{
		pZoneData = (DockData*)pZoneNode->data;
	}

	DockHostWindow_UpdateZoneSplitGrip(pDockHostWindow, pZoneNode, nDockSide, iDockSize);

	TreeNode* pLeaf = DockNode_Create(DockLayout_GetZoneStackGrip(nDockSide, iDockSize), DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, TRUE);
	if (!pLeaf || !pLeaf->data)
	{
		return FALSE;
	}

	DockData* pDockDataLeaf = (DockData*)pLeaf->data;
	DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
	DockData_PinHWND(pDockHostWindow, pDockDataLeaf, hWnd);

	pDockDataLeaf->nRole = DOCK_ROLE_PANEL;
	pDockDataLeaf->nPaneKind = nPaneKind;

	if (!pZoneNode->node1)
	{
		pZoneNode->node1 = pLeaf;
		if (pZoneData)
		{
			pZoneData->hWndActiveTab = hWnd;
		}
		return TRUE;
	}

	TreeNode* pSplit = DockNode_Create(DockLayout_GetZoneStackGrip(nDockSide, iDockSize), DockLayout_GetZoneStackStyle(nDockSide), FALSE);
	if (!pSplit || !pSplit->data)
	{
		if (pLeaf->data)
		{
			free(pLeaf->data);
		}
		free(pLeaf);
		return FALSE;
	}

	DockData* pDockDataSplit = (DockData*)pSplit->data;
	wcscpy_s(pDockDataSplit->lpszName, MAX_PATH, L"DockShell.ZoneStack");
	pDockDataSplit->nRole = DOCK_ROLE_ZONE_STACK_SPLIT;

	pSplit->node1 = pZoneNode->node1;
	pSplit->node2 = pLeaf;
	pZoneNode->node1 = pSplit;
	if (pZoneData)
	{
		pZoneData->hWndActiveTab = hWnd;
	}

	return TRUE;
}

int DockHostWindow_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen)
{
	return DockHostDrag_HitTestDockSide(pDockHostWindow, ptScreen);
}

static int DockHostWindow_HitTestSideInRect(const RECT* pRect, POINT pt, int iThresholdMin, int iThresholdMax)
{
	if (!pRect || !PtInRect(pRect, pt))
	{
		return DKS_NONE;
	}

	int width = pRect->right - pRect->left;
	int height = pRect->bottom - pRect->top;
	if (width <= 0 || height <= 0)
	{
		return DKS_NONE;
	}

	int threshold = min(width, height) / 4;
	threshold = max(threshold, iThresholdMin);
	threshold = min(threshold, iThresholdMax);

	int distLeft = pt.x - pRect->left;
	int distRight = pRect->right - pt.x;
	int distTop = pt.y - pRect->top;
	int distBottom = pRect->bottom - pt.y;

	int side = DKS_LEFT;
	int minDist = distLeft;
	if (distRight < minDist)
	{
		minDist = distRight;
		side = DKS_RIGHT;
	}
	if (distTop < minDist)
	{
		minDist = distTop;
		side = DKS_TOP;
	}
	if (distBottom < minDist)
	{
		minDist = distBottom;
		side = DKS_BOTTOM;
	}

	return (minDist > threshold) ? DKS_NONE : side;
}

BOOL DockHostWindow_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit)
{
	return DockHostDrag_HitTestDockTarget(pDockHostWindow, ptScreen, pTargetHit);
}

BOOL DockHostWindow_DockHWND(DockHostWindow* pDockHostWindow, HWND hWnd, int nDockSide, int iDockSize)
{
	return DockHostMutate_DockHWND(pDockHostWindow, hWnd, nDockSide, iDockSize);
}

static BOOL DockHostWindow_DockAroundPanel(DockHostWindow* pDockHostWindow, TreeNode* pAnchorNode, HWND hWnd, int nDockSide, int iDockSize)
{
	if (!pDockHostWindow || !pAnchorNode || !pAnchorNode->data || !hWnd || !IsWindow(hWnd))
	{
		return FALSE;
	}

	DockData* pAnchorData = (DockData*)pAnchorNode->data;
	if (!pAnchorData->hWnd || !IsWindow(pAnchorData->hWnd))
	{
		return FALSE;
	}

	DWORD dwSplitStyle = DGP_ABSOLUTE;
	switch (nDockSide)
	{
	case DKS_LEFT:
	case DKS_RIGHT:
		dwSplitStyle |= DGD_HORIZONTAL;
		break;

	case DKS_TOP:
	case DKS_BOTTOM:
		dwSplitStyle |= DGD_VERTICAL;
		break;

	default:
		return FALSE;
	}

	if (nDockSide == DKS_RIGHT || nDockSide == DKS_BOTTOM)
	{
		dwSplitStyle |= DGA_END;
	}
	else {
		dwSplitStyle |= DGA_START;
	}

	if (iDockSize <= 0)
	{
		iDockSize = 280;
	}

	TreeNode* pLeaf = DockNode_Create(DockLayout_GetZoneStackGrip(nDockSide, iDockSize), DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, TRUE);
	TreeNode* pSplit = DockNode_Create(DockLayout_GetZoneSplitGrip(nDockSide, iDockSize), dwSplitStyle, FALSE);
	if (!pLeaf || !pLeaf->data || !pSplit || !pSplit->data)
	{
		if (pLeaf)
		{
			if (pLeaf->data)
			{
				free(pLeaf->data);
			}
			free(pLeaf);
		}
		if (pSplit)
		{
			if (pSplit->data)
			{
				free(pSplit->data);
			}
			free(pSplit);
		}
		return FALSE;
	}

	DockData* pLeafData = (DockData*)pLeaf->data;
	DockPaneKind nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
	DockData_PinHWND(pDockHostWindow, pLeafData, hWnd);
	pLeafData->nRole = DOCK_ROLE_PANEL;
	pLeafData->nPaneKind = nPaneKind;

	DockData* pSplitData = (DockData*)pSplit->data;
	wcscpy_s(pSplitData->lpszName, MAX_PATH, L"DockShell.PanelSplit");
	pSplitData->rc = pAnchorData->rc;
	pSplitData->nRole = DOCK_ROLE_PANEL_SPLIT;

	if (nDockSide == DKS_LEFT || nDockSide == DKS_TOP)
	{
		pSplit->node1 = pLeaf;
		pSplit->node2 = pAnchorNode;
	}
	else {
		pSplit->node1 = pAnchorNode;
		pSplit->node2 = pLeaf;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	TreeNode* pParent = DockNode_FindParent(pRoot, pAnchorNode);
	if (pParent)
	{
		if (pParent->node1 == pAnchorNode)
		{
			pParent->node1 = pSplit;
		}
		else if (pParent->node2 == pAnchorNode) {
			pParent->node2 = pSplit;
		}
	}
	else if (pRoot == pAnchorNode) {
		DockHostWindow_SetRoot(pDockHostWindow, pSplit);
	}
	else {
		if (pLeaf->data)
		{
			free(pLeaf->data);
		}
		free(pLeaf);
		if (pSplit->data)
		{
			free(pSplit->data);
		}
		free(pSplit);
		return FALSE;
	}

	DockHostWindow_Rearrange(pDockHostWindow);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	return TRUE;
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

void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window)
{
	HWND hWnd = Window_GetHWND(window);
	DockData_PinHWND(pDockHostWindow, pDockData, hWnd);
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

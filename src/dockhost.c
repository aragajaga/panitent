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
#include "dockhostmutate.h"
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
#define DOCK_TARGET_GUIDE_SIZE 24
#define DOCK_TARGET_GUIDE_GAP 8
#define DOCK_TARGET_GUIDE_EDGE_MARGIN 12
#define WINDOWBUTTONSIZE 14
#define WINDOWBUTTONSPACING 3
#define DOCK_MIN_PANE_SIZE 96
static const WCHAR szAutoHideOverlayHostClassName[] = L"__DockAutoHideOverlayHost";

typedef struct DockHostWatermarkCache DockHostWatermarkCache;
struct DockHostWatermarkCache
{
	HBITMAP hSourceBitmap;
	HBITMAP hTintedBitmap;
	int width;
	int height;
	COLORREF tint;
};

static DockHostWatermarkCache g_dockHostWatermarkCache = { 0 };

static BOOL DockHostWindow_EnsureWatermarkCache(HDC hdc, int idBitmap, COLORREF tint);
static BOOL DockHostWindow_DrawMaskedBitmap(HDC hdc, const RECT* pDestRect, int idBitmap, COLORREF tint);
static BOOL DockHostWindow_OnDropFiles(void* pContext, HDROP hDrop);

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y);
BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y);
BOOL Dock_PinButtonHitTest(DockData* pDockData, int x, int y);
BOOL Dock_MoreButtonHitTest(DockData* pDockData, int x, int y);
void Dock_DestroyInclusive(TreeNode*, TreeNode*);
void DockNode_Paint(DockHostWindow* pDockHostWindow, TreeNode* pNodeParent, HDC hdc, HBRUSH hCaptionBrush);
static void DockHostWindow_DestroyDragOverlay(void);
static void DockHostWindow_ContinueFloatingDrag(HWND hWndFloating);
static void DockHostWindow_UpdateDragOverlayVisual(DockHostWindow* pDockHostWindow, int iRadius);
static BOOL DockNode_HasVisibleWindow(TreeNode* pNode);
static void DockHostWindow_AssignPersistentNameForHWND(DockData* pDockData, HWND hWnd);
static BOOL DockNode_IsStructural(TreeNode* pNode);
static BOOL DockNode_UsesProportionalGrip(TreeNode* pNode);
static BOOL DockHostWindow_DockIntoZone(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, HWND hWnd, int nDockSide, int iDockSize);
static void DockHostWindow_UpdateZoneSplitGrip(DockHostWindow* pDockHostWindow, TreeNode* pZoneNode, int nDockSide, int iDockSize);
static BOOL DockHostWindow_GetZoneTabRect(DockHostWindow* pDockHostWindow, int nDockSide, int iOffset, int iTabLength, RECT* pRect);
static int DockHostWindow_HitTestZoneTab(DockHostWindow* pDockHostWindow, int x, int y, HWND* phWndTab);
static void DockHostWindow_DrawZoneTabs(DockHostWindow* pDockHostWindow, HDC hdc);
static void DockHostWindow_GetPanelTabLabel(DockData* pDockData, WCHAR* pszLabel, int cchLabel);
static int DockHostWindow_GetZoneTabLengthFromLabel(HDC hdc, PCWSTR pszLabel);
static BOOL DockHostWindow_ToggleZoneCollapsed(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab);
static void DockHostWindow_UpdateZoneTabGutters(DockHostWindow* pDockHostWindow);
static void DockHostWindow_SyncZones(DockHostWindow* pDockHostWindow);
static BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect);
static BOOL DockHostWindow_GetAutoHideOverlayRect(DockHostWindow* pDockHostWindow, int nDockSide, RECT* pRect);
static BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow);
static BOOL DockHostWindow_ShowAutoHideOverlay(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab);
static void DockHostWindow_HideAutoHideOverlay(DockHostWindow* pDockHostWindow);
static void DockHostWindow_UpdateAutoHideOverlay(DockHostWindow* pDockHostWindow);
static BOOL DockHostWindow_BuildAutoHideOverlayLayout(DockHostWindow* pDockHostWindow, CaptionFrameLayout* pLayout);
static LRESULT CALLBACK DockAutoHideOverlayHostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void DockAutoHideOverlayHost_OnPaint(DockHostWindow* pDockHostWindow, HWND hWnd);
static BOOL DockHostWindow_TogglePanelPinned(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode);
static BOOL DockHostWindow_MovePanelToNewWindow(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode);
static void DockHostWindow_ShowPanelMenu(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode, POINT ptScreen);
static BOOL DockHostWindow_GetSplitRect(TreeNode* pNode, RECT* pRect);
static TreeNode* DockHostWindow_HitTestSplitGrip(DockHostWindow* pDockHostWindow, int x, int y);
static BOOL DockHostWindow_IsSplitVertical(TreeNode* pNode);
static int DockHostWindow_HitTestSideInRect(const RECT* pRect, POINT pt, int iThresholdMin, int iThresholdMax);
static void DockHostWindow_GetGlobalTargetGuideRects(const RECT* pHostClient, RECT* pRectTop, RECT* pRectLeft, RECT* pRectRight, RECT* pRectBottom);
static void DockHostWindow_GetLocalTargetGuideRects(const RECT* pHostClient, const RECT* pAnchorRect, RECT* pRectCenter, RECT* pRectTop, RECT* pRectLeft, RECT* pRectRight, RECT* pRectBottom);
static int DockHostWindow_HitTestGlobalTargetGuide(const RECT* pHostClient, POINT ptClient);
static int DockHostWindow_HitTestLocalTargetGuide(const RECT* pHostClient, const RECT* pAnchorRect, POINT ptClient);
static TreeNode* DockHostWindow_FindDockAnchorAtPoint(DockHostWindow* pDockHostWindow, POINT ptClient, RECT* pRectAnchor);
static BOOL DockHostWindow_DockAroundPanel(DockHostWindow* pDockHostWindow, TreeNode* pAnchorNode, HWND hWnd, int nDockSide, int iDockSize);
static void DockHostWindow_BeginSplitDrag(DockHostWindow* pDockHostWindow, TreeNode* pSplitNode, int x, int y);
static void DockHostWindow_UpdateSplitDrag(DockHostWindow* pDockHostWindow, int x, int y);
static void DockHostWindow_EndSplitDrag(DockHostWindow* pDockHostWindow);
static void DockHostWindow_DrawSplitGrips(DockHostWindow* pDockHostWindow, HDC hdc);
static void DockHostWindow_PaintContent(DockHostWindow* pDockHostWindow, HDC hdc, const RECT* pClientRect);
static BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd);
static int Dock_HitTypeToCaptionButton(int nHitType);
static void DockHostWindow_SetCaptionHotButton(DockHostWindow* pDockHostWindow, TreeNode* pNode, int nButton);
static void DockHostWindow_SetCaptionPressedButton(DockHostWindow* pDockHostWindow, TreeNode* pNode, int nButton);
static void DockHostWindow_ClearCaptionButtonState(DockHostWindow* pDockHostWindow);
static void DockHostWindow_SetAutoHideHotButton(DockHostWindow* pDockHostWindow, int nButton);
static void DockHostWindow_SetAutoHidePressedButton(DockHostWindow* pDockHostWindow, int nButton);
static void DockHostWindow_ClearAutoHideCaptionState(DockHostWindow* pDockHostWindow);
DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd);

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
void DockHostWindow_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags);
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

#define DCB_NONE 0
#define DCB_CLOSE 1
#define DCB_PIN 2
#define DCB_MORE 3

static int Dock_HitTypeToCaptionButton(int nHitType)
{
	switch (nHitType)
	{
	case DHT_CLOSEBTN:
		return DCB_CLOSE;
	case DHT_PINBTN:
		return DCB_PIN;
	case DHT_MOREBTN:
		return DCB_MORE;
	default:
		return DCB_NONE;
	}
}

static void DockHostWindow_SetCaptionHotButton(DockHostWindow* pDockHostWindow, TreeNode* pNode, int nButton)
{
	if (!pDockHostWindow)
	{
		return;
	}

	if (nButton == DCB_NONE)
	{
		pNode = NULL;
	}

	if (pDockHostWindow->pCaptionHotNode == pNode &&
		pDockHostWindow->nCaptionHotButton == nButton)
	{
		return;
	}

	pDockHostWindow->pCaptionHotNode = pNode;
	pDockHostWindow->nCaptionHotButton = nButton;
	Window_Invalidate((Window*)pDockHostWindow);
}

static void DockHostWindow_SetCaptionPressedButton(DockHostWindow* pDockHostWindow, TreeNode* pNode, int nButton)
{
	if (!pDockHostWindow)
	{
		return;
	}

	if (nButton == DCB_NONE)
	{
		pNode = NULL;
	}

	if (pDockHostWindow->pCaptionPressedNode == pNode &&
		pDockHostWindow->nCaptionPressedButton == nButton)
	{
		return;
	}

	pDockHostWindow->pCaptionPressedNode = pNode;
	pDockHostWindow->nCaptionPressedButton = nButton;
	Window_Invalidate((Window*)pDockHostWindow);
}

static void DockHostWindow_ClearCaptionButtonState(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		return;
	}

	BOOL bChanged =
		(pDockHostWindow->pCaptionHotNode != NULL) ||
		(pDockHostWindow->nCaptionHotButton != DCB_NONE) ||
		(pDockHostWindow->pCaptionPressedNode != NULL) ||
		(pDockHostWindow->nCaptionPressedButton != DCB_NONE);

	pDockHostWindow->pCaptionHotNode = NULL;
	pDockHostWindow->nCaptionHotButton = DCB_NONE;
	pDockHostWindow->pCaptionPressedNode = NULL;
	pDockHostWindow->nCaptionPressedButton = DCB_NONE;

	if (bChanged)
	{
		Window_Invalidate((Window*)pDockHostWindow);
	}
}

static void DockHostWindow_SetAutoHideHotButton(DockHostWindow* pDockHostWindow, int nButton)
{
	if (!pDockHostWindow)
	{
		return;
	}

	if (pDockHostWindow->nAutoHideOverlayHotButton == nButton)
	{
		return;
	}

	pDockHostWindow->nAutoHideOverlayHotButton = nButton;
	if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
	{
		InvalidateRect(pDockHostWindow->hWndAutoHideOverlayHost, NULL, FALSE);
	}
}

static void DockHostWindow_SetAutoHidePressedButton(DockHostWindow* pDockHostWindow, int nButton)
{
	if (!pDockHostWindow)
	{
		return;
	}

	if (pDockHostWindow->nAutoHideOverlayPressedButton == nButton)
	{
		return;
	}

	pDockHostWindow->nAutoHideOverlayPressedButton = nButton;
	if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
	{
		InvalidateRect(pDockHostWindow->hWndAutoHideOverlayHost, NULL, FALSE);
	}
}

static void DockHostWindow_ClearAutoHideCaptionState(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		return;
	}

	BOOL bChanged =
		pDockHostWindow->fAutoHideOverlayTrackMouse ||
		pDockHostWindow->nAutoHideOverlayHotButton != DCB_NONE ||
		pDockHostWindow->nAutoHideOverlayPressedButton != DCB_NONE;
	pDockHostWindow->fAutoHideOverlayTrackMouse = FALSE;
	pDockHostWindow->nAutoHideOverlayHotButton = DCB_NONE;
	pDockHostWindow->nAutoHideOverlayPressedButton = DCB_NONE;
	if (bChanged && pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
	{
		InvalidateRect(pDockHostWindow->hWndAutoHideOverlayHost, NULL, FALSE);
	}
}

static void Dock_GetCaptionMetrics(CaptionFrameMetrics* pMetrics)
{
	if (!pMetrics)
	{
		return;
	}

	pMetrics->borderSize = DOCK_CAPTION_INSET;
	pMetrics->captionHeight = iCaptionHeight;
	pMetrics->buttonSpacing = WINDOWBUTTONSPACING;
	pMetrics->textPaddingLeft = DOCK_CAPTION_INSET;
	pMetrics->textPaddingRight = DOCK_CAPTION_INSET;
	pMetrics->textPaddingY = 0;
}

static int Dock_BuildCaptionButtons(DockData* pDockData, BOOL bPinFirst, int iPinGlyph, BOOL bIncludeChevron, CaptionButton* pButtons, int cButtons)
{
	if (!pDockData || !pButtons || cButtons <= 0)
	{
		return 0;
	}

	BOOL bCanClose = DockPolicy_CanClosePanel(pDockData->nRole, pDockData->lpszName);
	BOOL bCanPin = DockPolicy_CanPinPanel(pDockData->nRole, pDockData->lpszName);
	int nCount = 0;

	if (bPinFirst && bCanPin && nCount < cButtons)
	{
		pButtons[nCount++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, iPinGlyph, DCB_PIN };
	}
	if (bCanClose && nCount < cButtons)
	{
		pButtons[nCount++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, CAPTION_GLYPH_CLOSE_TILE, DCB_CLOSE };
	}
	if (!bPinFirst && bCanPin && nCount < cButtons)
	{
		pButtons[nCount++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, iPinGlyph, DCB_PIN };
	}
	if (bIncludeChevron && nCount < cButtons)
	{
		pButtons[nCount++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, CAPTION_GLYPH_CHEVRON_TILE, DCB_MORE };
	}

	return nCount;
}

static BOOL Dock_BuildCaptionLayout(DockData* pDockData, BOOL bPinFirst, CaptionFrameLayout* pLayout)
{
	if (!pDockData || !pLayout || !pDockData->bShowCaption)
	{
		return FALSE;
	}

	CaptionFrameMetrics metrics = { 0 };
	Dock_GetCaptionMetrics(&metrics);

	CaptionButton buttons[3] = { 0 };
	int nButtons = Dock_BuildCaptionButtons(
		pDockData,
		bPinFirst,
		bPinFirst ? CAPTION_GLYPH_PIN_DIAGONAL_TILE : CAPTION_GLYPH_PIN_VERTICAL_TILE,
		TRUE,
		buttons,
		ARRAYSIZE(buttons));
	return CaptionFrame_BuildLayout(&pDockData->rc, &metrics, buttons, nButtons, pLayout);
}

static BOOL Dock_GetCaptionButtonRect(DockData* pDockData, int nButtonType, RECT* pRect)
{
	if (!pDockData || !pRect || nButtonType == DCB_NONE)
	{
		return FALSE;
	}

	CaptionFrameLayout layout = { 0 };
	if (!Dock_BuildCaptionLayout(pDockData, FALSE, &layout))
	{
		return FALSE;
	}

	return CaptionFrame_GetButtonRect(&layout, nButtonType, pRect);
}

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y)
{
	CaptionFrameLayout layout = { 0 };
	if (!Dock_BuildCaptionLayout(pDockData, FALSE, &layout))
	{
		return FALSE;
	}

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	return PtInRect(&layout.rcCaption, pt);
}

BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y)
{
	RECT rcButton = { 0 };
	if (!Dock_GetCaptionButtonRect(pDockData, DCB_CLOSE, &rcButton))
	{
		return FALSE;
	}

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	return PtInRect(&rcButton, pt);
}

BOOL Dock_PinButtonHitTest(DockData* pDockData, int x, int y)
{
	RECT rcButton = { 0 };
	if (!Dock_GetCaptionButtonRect(pDockData, DCB_PIN, &rcButton))
	{
		return FALSE;
	}

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	return PtInRect(&rcButton, pt);
}

BOOL Dock_MoreButtonHitTest(DockData* pDockData, int x, int y)
{
	RECT rcButton = { 0 };
	if (!Dock_GetCaptionButtonRect(pDockData, DCB_MORE, &rcButton))
	{
		return FALSE;
	}

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	return PtInRect(&rcButton, pt);
}

int DockHostWindow_HitTest(DockHostWindow* pDockHostWindow, TreeNode** ppTreeNode, int x, int y)
{
	if (!pDockHostWindow->pRoot_)
	{
		return 0;
	}

	TreeTraversalRLOT traversal = { 0 };
	TreeTraversalRLOT_Init(&traversal, pDockHostWindow->pRoot_);
	
	TreeNode* pCurrentNode = NULL;
	TreeNode* pHittedNode = NULL;
	while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
	{
		DockData* pDockData = (DockData*)pCurrentNode->data;

		POINT pt = { 0 };
		pt.x = x;
		pt.y = y;
		if (PtInRect(&pDockData->rc, pt))
		{
			pHittedNode = pCurrentNode;
			break;
		}
	}
	TreeTraversalRLOT_Destroy(&traversal);

	/*
	int localX = x - pHittedDock->rc.left;
	int localY = y - pHittedDock->rc.top;
	*/

	*ppTreeNode = pHittedNode;

	if (pHittedNode && pHittedNode->data)
	{
		DockData* pDockDataHit = (DockData*)pHittedNode->data;
		if (Dock_CloseButtonHitTest(pDockDataHit, x, y) && DockPolicy_CanClosePanel(pDockDataHit->nRole, pDockDataHit->lpszName))
		{
			return DHT_CLOSEBTN;
		}
		else if (Dock_PinButtonHitTest(pDockDataHit, x, y) && DockPolicy_CanPinPanel(pDockDataHit->nRole, pDockDataHit->lpszName))
		{
			return DHT_PINBTN;
		}
		else if (Dock_MoreButtonHitTest(pDockDataHit, x, y))
		{
			return DHT_MOREBTN;
		}
		else if (Dock_CaptionHitTest(pDockDataHit, x, y) && DockPolicy_CanUndockPanel(pDockDataHit->nRole, pDockDataHit->lpszName))
		{
			return DHT_CAPTION;
		}
	}

	return DHT_UNKNOWN;
}

static BOOL DockNode_HasVisibleWindow(TreeNode* pNode)
{
	return DockHostLayout_NodeHasVisibleWindow(pNode);
}

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

static int DockHostWindow_GetZoneSideTabGutter(DockHostWindow* pDockHostWindow, int nDockSide)
{
	TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, nDockSide);
	if (!pZoneNode)
	{
		return 0;
	}

	TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
	int nTabs = DockZone_GetPanelsByCollapsed(pZoneNode, tabs, ARRAYSIZE(tabs), TRUE);
	return (nTabs > 0) ? iZoneTabGutter : 0;
}

static void DockHostWindow_UpdateZoneTabGutters(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		g_iZoneTabGutterLeft = 0;
		g_iZoneTabGutterRight = 0;
		g_iZoneTabGutterTop = 0;
		g_iZoneTabGutterBottom = 0;
		return;
	}

	g_iZoneTabGutterLeft = DockHostWindow_GetZoneSideTabGutter(pDockHostWindow, DKS_LEFT);
	g_iZoneTabGutterRight = DockHostWindow_GetZoneSideTabGutter(pDockHostWindow, DKS_RIGHT);
	g_iZoneTabGutterTop = DockHostWindow_GetZoneSideTabGutter(pDockHostWindow, DKS_TOP);
	g_iZoneTabGutterBottom = DockHostWindow_GetZoneSideTabGutter(pDockHostWindow, DKS_BOTTOM);
}

static void DockHostWindow_SyncZones(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		return;
	}

	const int sides[] = { DKS_LEFT, DKS_RIGHT, DKS_TOP, DKS_BOTTOM };
	for (int i = 0; i < ARRAYSIZE(sides); ++i)
	{
		TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, sides[i]);
		DockZone_EnsureActiveTab(pZoneNode);
	}

	DockHostWindow_UpdateZoneTabGutters(pDockHostWindow);
}

static BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect)
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

static BOOL DockHostWindow_GetAutoHideOverlayRect(DockHostWindow* pDockHostWindow, int nDockSide, RECT* pRect)
{
	if (!pDockHostWindow || !pRect)
	{
		return FALSE;
	}

	TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, nDockSide);
	if (!pZoneNode || !pZoneNode->data)
	{
		return FALSE;
	}

	RECT rcContent = { 0 };
	if (!DockHostWindow_GetHostContentRect(pDockHostWindow, &rcContent))
	{
		return FALSE;
	}

	int iSpan = (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT) ?
		Win32_Rect_GetWidth(&rcContent) : Win32_Rect_GetHeight(&rcContent);
	if (iSpan <= 0)
	{
		return FALSE;
	}

	int iOverlaySize = DockLayout_GetZoneSplitGrip(nDockSide, 0);
	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	TreeNode* pParent = DockNode_FindParent(pRoot, pZoneNode);
	if (pParent && pParent->data)
	{
		iOverlaySize = ((DockData*)pParent->data)->iGripPos;
	}

	iOverlaySize = DockLayout_ClampSplitGrip(iSpan, iOverlaySize, 96);
	iOverlaySize = max(96, min(iOverlaySize, iSpan));

	*pRect = rcContent;
	switch (nDockSide)
	{
	case DKS_LEFT:
		pRect->right = min(pRect->left + iOverlaySize, rcContent.right);
		break;
	case DKS_RIGHT:
		pRect->left = max(pRect->right - iOverlaySize, rcContent.left);
		break;
	case DKS_TOP:
		pRect->bottom = min(pRect->top + iOverlaySize, rcContent.bottom);
		break;
	case DKS_BOTTOM:
		pRect->top = max(pRect->bottom - iOverlaySize, rcContent.top);
		break;
	default:
		return FALSE;
	}

	return Win32_Rect_GetWidth(pRect) > 0 && Win32_Rect_GetHeight(pRect) > 0;
}

static BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		return FALSE;
	}

	if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
	{
		return TRUE;
	}

	HWND hWndDockHost = Window_GetHWND((Window*)pDockHostWindow);
	if (!hWndDockHost || !IsWindow(hWndDockHost))
	{
		return FALSE;
	}

	WNDCLASSEX wcex = { 0 };
	if (!GetClassInfoEx(GetModuleHandle(NULL), szAutoHideOverlayHostClassName, &wcex))
	{
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = DockAutoHideOverlayHostWndProc;
		wcex.hInstance = GetModuleHandle(NULL);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wcex.lpszClassName = szAutoHideOverlayHostClassName;
		if (!RegisterClassEx(&wcex))
		{
			return FALSE;
		}
	}

	HWND hWndOverlayHost = CreateWindowEx(
		WS_EX_CONTROLPARENT,
		szAutoHideOverlayHostClassName,
		L"",
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 0, 0,
		hWndDockHost,
		NULL,
		GetModuleHandle(NULL),
		(LPVOID)pDockHostWindow);
	if (!hWndOverlayHost || !IsWindow(hWndOverlayHost))
	{
		return FALSE;
	}

	pDockHostWindow->hWndAutoHideOverlayHost = hWndOverlayHost;
	ShowWindow(hWndOverlayHost, SW_HIDE);
	return TRUE;
}

static void DockHostWindow_HideAutoHideOverlay(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow)
	{
		return;
	}

	HWND hWndDockHost = Window_GetHWND((Window*)pDockHostWindow);
	HWND hWndOverlayHost = pDockHostWindow->hWndAutoHideOverlayHost;

	if (pDockHostWindow->fAutoHideOverlayVisible &&
		pDockHostWindow->hWndAutoHideOverlay &&
		IsWindow(pDockHostWindow->hWndAutoHideOverlay))
	{
		if (hWndDockHost && IsWindow(hWndDockHost) &&
			hWndOverlayHost && IsWindow(hWndOverlayHost) &&
			GetParent(pDockHostWindow->hWndAutoHideOverlay) == hWndOverlayHost)
		{
			SetParent(pDockHostWindow->hWndAutoHideOverlay, hWndDockHost);
		}

		ShowWindow(pDockHostWindow->hWndAutoHideOverlay, SW_HIDE);
	}

	if (hWndOverlayHost && IsWindow(hWndOverlayHost))
	{
		ShowWindow(hWndOverlayHost, SW_HIDE);
	}

	pDockHostWindow->fAutoHideOverlayVisible = FALSE;
	pDockHostWindow->nAutoHideOverlaySide = DKS_NONE;
	pDockHostWindow->hWndAutoHideOverlay = NULL;
	SetRectEmpty(&pDockHostWindow->rcAutoHideOverlay);
	DockHostWindow_ClearAutoHideCaptionState(pDockHostWindow);
	Window_Invalidate((Window*)pDockHostWindow);
}

static void DockHostWindow_UpdateAutoHideOverlay(DockHostWindow* pDockHostWindow)
{
	if (!pDockHostWindow || !pDockHostWindow->fAutoHideOverlayVisible)
	{
		return;
	}

	HWND hWndOverlay = pDockHostWindow->hWndAutoHideOverlay;
	if (!hWndOverlay || !IsWindow(hWndOverlay))
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	TreeNode* pPanelNode = DockNode_FindByHWND(pRoot, hWndOverlay);
	if (!pPanelNode || !pPanelNode->data)
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	if (!pPanelData->bCollapsed)
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return;
	}

	int nDockSide = DockHostWindow_GetPanelDockSide(pDockHostWindow, hWndOverlay);
	if (nDockSide == DKS_NONE)
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return;
	}
	pDockHostWindow->nAutoHideOverlaySide = nDockSide;

	if (!DockHostWindow_EnsureAutoHideOverlayHost(pDockHostWindow))
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return;
	}

	RECT rcOverlay = { 0 };
	if (!DockHostWindow_GetAutoHideOverlayRect(pDockHostWindow, nDockSide, &rcOverlay))
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
		return;
	}

	pDockHostWindow->hWndAutoHideOverlay = hWndOverlay;
	pDockHostWindow->rcAutoHideOverlay = rcOverlay;

	HWND hWndOverlayHost = pDockHostWindow->hWndAutoHideOverlayHost;
	RECT rcHost = rcOverlay;
	int hostWidth = Win32_Rect_GetWidth(&rcHost);
	int hostHeight = Win32_Rect_GetHeight(&rcHost);
	RECT rcChild = { 0, 0, hostWidth, hostHeight };
	rcChild.top += iCaptionHeight + 1;
	Win32_ContractRect(&rcChild, 4, 4);
	if (Win32_Rect_GetWidth(&rcChild) <= 4 || Win32_Rect_GetHeight(&rcChild) <= 4)
	{
		ShowWindow(hWndOverlay, SW_HIDE);
		if (hWndOverlayHost && IsWindow(hWndOverlayHost))
		{
			ShowWindow(hWndOverlayHost, SW_HIDE);
		}
		return;
	}

	if (GetParent(hWndOverlay) != hWndOverlayHost)
	{
		SetParent(hWndOverlay, hWndOverlayHost);
	}

	SetWindowPos(hWndOverlayHost, HWND_TOP, rcHost.left, rcHost.top,
		hostWidth, hostHeight,
		SWP_NOACTIVATE | SWP_SHOWWINDOW);

	SetWindowPos(hWndOverlay, HWND_TOP, rcChild.left, rcChild.top,
		Win32_Rect_GetWidth(&rcChild), Win32_Rect_GetHeight(&rcChild),
		SWP_NOACTIVATE | SWP_SHOWWINDOW);
	InvalidateRect(hWndOverlayHost, NULL, FALSE);
}

static BOOL DockHostWindow_BuildAutoHideOverlayLayout(DockHostWindow* pDockHostWindow, CaptionFrameLayout* pLayout)
{
	if (!pDockHostWindow || !pLayout || !pDockHostWindow->fAutoHideOverlayVisible)
	{
		return FALSE;
	}

	HWND hWndOverlayHost = pDockHostWindow->hWndAutoHideOverlayHost;
	if (!hWndOverlayHost || !IsWindow(hWndOverlayHost))
	{
		return FALSE;
	}

	BOOL bShowPin = TRUE;
	BOOL bShowClose = TRUE;
	if (pDockHostWindow->hWndAutoHideOverlay && IsWindow(pDockHostWindow->hWndAutoHideOverlay))
	{
		TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
		TreeNode* pPanelNode = DockNode_FindByHWND(pRoot, pDockHostWindow->hWndAutoHideOverlay);
		if (pPanelNode && pPanelNode->data)
		{
			DockData* pPanelData = (DockData*)pPanelNode->data;
			bShowPin = DockPolicy_CanPinPanel(pPanelData->nRole, pPanelData->lpszName);
			bShowClose = DockPolicy_CanClosePanel(pPanelData->nRole, pPanelData->lpszName);
		}
	}

	CaptionButton buttons[3] = { 0 };
	int nButtons = 0;

	/* Keep button ordering consistent with pinned/floating: close, pin, chevron (right to left in layout builder). */
	if (bShowClose && nButtons < ARRAYSIZE(buttons))
	{
		buttons[nButtons++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, CAPTION_GLYPH_CLOSE_TILE, DCB_CLOSE };
	}
	if (bShowPin && nButtons < ARRAYSIZE(buttons))
	{
		buttons[nButtons++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, CAPTION_GLYPH_PIN_DIAGONAL_TILE, DCB_PIN };
	}
	if (nButtons < ARRAYSIZE(buttons))
	{
		buttons[nButtons++] = (CaptionButton){ (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE }, CAPTION_GLYPH_CHEVRON_TILE, DCB_MORE };
	}

	CaptionFrameMetrics metrics = { 0 };
	Dock_GetCaptionMetrics(&metrics);
	RECT rcClient = { 0 };
	GetClientRect(hWndOverlayHost, &rcClient);
	return CaptionFrame_BuildLayout(&rcClient, &metrics, buttons, nButtons, pLayout);
}

static void DockAutoHideOverlayHost_OnPaint(DockHostWindow* pDockHostWindow, HWND hWnd)
{
	if (!pDockHostWindow || !hWnd || !IsWindow(hWnd))
	{
		return;
	}

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

	CaptionFrameLayout layout = { 0 };
	if (DockHostWindow_BuildAutoHideOverlayLayout(pDockHostWindow, &layout))
	{
		WCHAR szLabel[MAX_PATH] = L"";
		if (pDockHostWindow->hWndAutoHideOverlay && IsWindow(pDockHostWindow->hWndAutoHideOverlay))
		{
			GetWindowText(pDockHostWindow->hWndAutoHideOverlay, szLabel, ARRAYSIZE(szLabel));
		}

		CaptionFramePalette palette = { 0 };
		PanitentTheme_GetCaptionPalette(&palette);
		CaptionFrame_DrawStateful(
			hdcTarget,
			&layout,
			&palette,
			szLabel,
			PanitentApp_GetUIFont(PanitentApp_Instance()),
			pDockHostWindow->nAutoHideOverlayHotButton,
			pDockHostWindow->nAutoHideOverlayPressedButton);
	}

	if (hdcTarget == hdcBuffer)
	{
		BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);
		SelectObject(hdcBuffer, hOldBitmap);
		DeleteObject(hbmBuffer);
		DeleteDC(hdcBuffer);
	}
	else if (hdcBuffer) {
		DeleteDC(hdcBuffer);
	}

	EndPaint(hWnd, &ps);
}

static LRESULT CALLBACK DockAutoHideOverlayHostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DockHostWindow* pDockHostWindow = (DockHostWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_NCCREATE:
	{
		CREATESTRUCT* pCreateStruct = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreateStruct->lpCreateParams);
		return TRUE;
	}

	case WM_ERASEBKGND:
		/* Full background is painted in WM_PAINT using a backbuffer. */
		return 1;

	case WM_MOUSEMOVE:
	{
		if (!pDockHostWindow || !pDockHostWindow->fAutoHideOverlayVisible)
		{
			return 0;
		}

		if (!pDockHostWindow->fAutoHideOverlayTrackMouse)
		{
			TRACKMOUSEEVENT tme = { 0 };
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			if (TrackMouseEvent(&tme))
			{
				pDockHostWindow->fAutoHideOverlayTrackMouse = TRUE;
			}
		}

		CaptionFrameLayout layout = { 0 };
		if (!DockHostWindow_BuildAutoHideOverlayLayout(pDockHostWindow, &layout))
		{
			DockHostWindow_SetAutoHideHotButton(pDockHostWindow, DCB_NONE);
			return 0;
		}

		POINT pt = { (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam) };
		int nHitButton = CaptionFrame_HitTestButton(&layout, pt);
		DockHostWindow_SetAutoHideHotButton(pDockHostWindow, nHitButton);
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		if (!pDockHostWindow || !pDockHostWindow->fAutoHideOverlayVisible)
		{
			return 0;
		}

		CaptionFrameLayout layout = { 0 };
		if (!DockHostWindow_BuildAutoHideOverlayLayout(pDockHostWindow, &layout))
		{
			return 0;
		}

		POINT pt = { (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam) };
		int nHitButton = CaptionFrame_HitTestButton(&layout, pt);
		if (nHitButton == DCB_CLOSE || nHitButton == DCB_PIN || nHitButton == DCB_MORE)
		{
			DockHostWindow_SetAutoHideHotButton(pDockHostWindow, nHitButton);
			DockHostWindow_SetAutoHidePressedButton(pDockHostWindow, nHitButton);
			SetCapture(hWnd);
			return 0;
		}

		if (pDockHostWindow->hWndAutoHideOverlay && IsWindow(pDockHostWindow->hWndAutoHideOverlay))
		{
			SetFocus(pDockHostWindow->hWndAutoHideOverlay);
		}
		return 0;
	}

	case WM_LBUTTONUP:
	{
		if (!pDockHostWindow)
		{
			return 0;
		}

		int nPressedButton = pDockHostWindow->nAutoHideOverlayPressedButton;
		if (nPressedButton == DCB_NONE)
		{
			return 0;
		}

		if (GetCapture() == hWnd)
		{
			ReleaseCapture();
		}

		CaptionFrameLayout layout = { 0 };
		int nHitButton = DCB_NONE;
		if (DockHostWindow_BuildAutoHideOverlayLayout(pDockHostWindow, &layout))
		{
			POINT pt = { (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam) };
			nHitButton = CaptionFrame_HitTestButton(&layout, pt);
			DockHostWindow_SetAutoHideHotButton(pDockHostWindow, nHitButton);
		}

		DockHostWindow_SetAutoHidePressedButton(pDockHostWindow, DCB_NONE);
		if (nHitButton != nPressedButton)
		{
			return 0;
		}

		TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
		TreeNode* pPanelNode = DockNode_FindByHWND(pRoot, pDockHostWindow->hWndAutoHideOverlay);

		if (nPressedButton == DCB_PIN)
		{
			if (pPanelNode)
			{
				DockHostWindow_TogglePanelPinned(pDockHostWindow, pPanelNode);
			}
			else {
				DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
			}
			return 0;
		}

		if (nPressedButton == DCB_CLOSE)
		{
			DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
			if (pPanelNode)
			{
				DockHostWindow_DestroyInclusive(pDockHostWindow, pPanelNode);
			}
			return 0;
		}

		if (nPressedButton == DCB_MORE)
		{
			POINT ptScreen = { (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam) };
			ClientToScreen(hWnd, &ptScreen);
			DockHostWindow_ShowPanelMenu(pDockHostWindow, pPanelNode, ptScreen);
			return 0;
		}

		return 0;
	}

	case WM_MOUSELEAVE:
		if (pDockHostWindow)
		{
			pDockHostWindow->fAutoHideOverlayTrackMouse = FALSE;
			DockHostWindow_SetAutoHideHotButton(pDockHostWindow, DCB_NONE);
		}
		return 0;

	case WM_CAPTURECHANGED:
		if (pDockHostWindow)
		{
			DockHostWindow_SetAutoHidePressedButton(pDockHostWindow, DCB_NONE);
		}
		return 0;

	case WM_PAINT:
		DockAutoHideOverlayHost_OnPaint(pDockHostWindow, hWnd);
		return 0;

	case WM_NCDESTROY:
		SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
		break;
	}

	UNREFERENCED_PARAMETER(wParam);
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static BOOL DockHostWindow_ShowAutoHideOverlay(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab)
{
	if (!pDockHostWindow || !hWndTab || !IsWindow(hWndTab))
	{
		return FALSE;
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	TreeNode* pPanelNode = DockNode_FindByHWND(pRoot, hWndTab);
	if (!pPanelNode || !pPanelNode->data)
	{
		return FALSE;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	if (!pPanelData->bCollapsed)
	{
		return FALSE;
	}

	int nPanelDockSide = DockHostWindow_GetPanelDockSide(pDockHostWindow, hWndTab);
	if (nPanelDockSide == DKS_NONE)
	{
		return FALSE;
	}
	nDockSide = nPanelDockSide;

	HWND hWndHost = Window_GetHWND((Window*)pDockHostWindow);
	if (!hWndHost || !IsWindow(hWndHost))
	{
		return FALSE;
	}

	if (pDockHostWindow->fAutoHideOverlayVisible)
	{
		DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
	}

	pDockHostWindow->fAutoHideOverlayVisible = TRUE;
	pDockHostWindow->nAutoHideOverlaySide = nDockSide;
	pDockHostWindow->hWndAutoHideOverlay = hWndTab;
	DockHostWindow_ClearAutoHideCaptionState(pDockHostWindow);

	DockHostWindow_UpdateAutoHideOverlay(pDockHostWindow);
	Window_Invalidate((Window*)pDockHostWindow);
	return TRUE;
}

static BOOL DockHostWindow_TogglePanelPinned(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode)
{
	if (!pDockHostWindow || !pPanelNode || !pPanelNode->data)
	{
		return FALSE;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	if (!DockPolicy_CanPinPanel(pPanelData->nRole, pPanelData->lpszName))
	{
		return FALSE;
	}

	TreeNode* pZoneNode = DockHostWindow_FindOwningZoneNode(pDockHostWindow, pPanelNode);
	if (!pZoneNode || !pZoneNode->data)
	{
		return FALSE;
	}

	DockData* pZoneData = (DockData*)pZoneNode->data;
	BOOL bCollapse = pPanelData->bCollapsed ? FALSE : TRUE;
	pPanelData->bCollapsed = bCollapse;

	if (pPanelData->hWnd && IsWindow(pPanelData->hWnd) && !bCollapse)
	{
		pZoneData->hWndActiveTab = pPanelData->hWnd;
	}

	if (pPanelData->hWnd && IsWindow(pPanelData->hWnd) &&
		bCollapse &&
		pZoneData->hWndActiveTab == pPanelData->hWnd)
	{
		TreeNode* visibleTabs[DOCK_ZONE_MAX_TABS] = { 0 };
		int nVisibleTabs = DockZone_GetPanelsByCollapsed(pZoneNode, visibleTabs, ARRAYSIZE(visibleTabs), FALSE);
		if (nVisibleTabs > 0 && visibleTabs[0] && visibleTabs[0]->data)
		{
			pZoneData->hWndActiveTab = ((DockData*)visibleTabs[0]->data)->hWnd;
		}
		else {
			pZoneData->hWndActiveTab = NULL;
		}
	}

	DockHostWindow_HideAutoHideOverlay(pDockHostWindow);

	DockHostWindow_Rearrange(pDockHostWindow);
	return TRUE;
}

#define IDM_PANEL_DOCK 4101
#define IDM_PANEL_AUTOHIDE 4102
#define IDM_PANEL_MOVETONEW 4103
#define IDM_PANEL_CLOSE 4104

static BOOL DockHostWindow_MovePanelToNewWindow(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode)
{
	if (!pDockHostWindow || !pPanelNode || !pPanelNode->data)
	{
		return FALSE;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	HWND hWndPanel = pPanelData->hWnd;
	if (!hWndPanel || !IsWindow(hWndPanel))
	{
		return FALSE;
	}

	RECT rcPanelLocal = pPanelData->rc;
	RECT rcPanelScreen = rcPanelLocal;
	MapWindowPoints(Window_GetHWND((Window*)pDockHostWindow), HWND_DESKTOP, (POINT*)&rcPanelScreen, 2);

	DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
	DockHostWindow_Undock(pDockHostWindow, pPanelNode);

	FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
	HWND hWndFloating = Window_CreateWindow((Window*)pFloatingWindowContainer, NULL);
	FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
	FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndPanel);

	int width = max(Win32_Rect_GetWidth(&rcPanelScreen), 260);
	int height = max(Win32_Rect_GetHeight(&rcPanelScreen), 220);
	int x = rcPanelScreen.left;
	int y = rcPanelScreen.top;
	SetWindowPos(hWndFloating, NULL, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

	DockHostWindow_Rearrange(pDockHostWindow);
	return TRUE;
}

static void DockHostWindow_ShowPanelMenu(DockHostWindow* pDockHostWindow, TreeNode* pPanelNode, POINT ptScreen)
{
	if (!pDockHostWindow || !pPanelNode || !pPanelNode->data)
	{
		return;
	}

	DockData* pPanelData = (DockData*)pPanelNode->data;
	BOOL bCanPin = DockPolicy_CanPinPanel(pPanelData->nRole, pPanelData->lpszName);
	BOOL bCanUndock = DockPolicy_CanUndockPanel(pPanelData->nRole, pPanelData->lpszName);
	BOOL bCanClose = DockPolicy_CanClosePanel(pPanelData->nRole, pPanelData->lpszName);
	BOOL bAutoHide = pPanelData->bCollapsed ? TRUE : FALSE;

	HMENU hMenu = CreatePopupMenu();
	if (!hMenu)
	{
		return;
	}

	AppendMenu(hMenu, MF_STRING | (bAutoHide ? MF_ENABLED : MF_GRAYED), IDM_PANEL_DOCK, L"Doc&k");
	AppendMenu(hMenu, MF_STRING | ((bCanPin && !bAutoHide) ? MF_ENABLED : MF_GRAYED), IDM_PANEL_AUTOHIDE, L"&Auto Hide");
	AppendMenu(hMenu, MF_STRING | (bCanUndock ? MF_ENABLED : MF_GRAYED), IDM_PANEL_MOVETONEW, L"Move To &New Window");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | (bCanClose ? MF_ENABLED : MF_GRAYED), IDM_PANEL_CLOSE, L"&Close\tShift+Esc");

	UINT uCmd = (UINT)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN,
		ptScreen.x, ptScreen.y, 0, Window_GetHWND((Window*)pDockHostWindow), NULL);
	DestroyMenu(hMenu);

	switch (uCmd)
	{
	case IDM_PANEL_DOCK:
		if (bAutoHide)
		{
			DockHostWindow_TogglePanelPinned(pDockHostWindow, pPanelNode);
		}
		break;

	case IDM_PANEL_AUTOHIDE:
		if (bCanPin && !bAutoHide)
		{
			DockHostWindow_TogglePanelPinned(pDockHostWindow, pPanelNode);
		}
		break;

	case IDM_PANEL_MOVETONEW:
		if (bCanUndock)
		{
			DockHostWindow_MovePanelToNewWindow(pDockHostWindow, pPanelNode);
		}
		break;

	case IDM_PANEL_CLOSE:
		if (bCanClose)
		{
			DockHostWindow_DestroyInclusive(pDockHostWindow, pPanelNode);
		}
		break;
	}
}

static void DockHostWindow_GetPanelTabLabel(DockData* pDockData, WCHAR* pszLabel, int cchLabel)
{
	if (!pszLabel || cchLabel <= 0)
	{
		return;
	}

	wcsncpy_s(pszLabel, cchLabel, L"Tab", _TRUNCATE);
	if (!pDockData)
	{
		return;
	}

	PCWSTR pszSource = pDockData->lpszCaption[0] ? pDockData->lpszCaption : pDockData->lpszName;
	if (pszSource && pszSource[0])
	{
		wcsncpy_s(pszLabel, cchLabel, pszSource, _TRUNCATE);
	}
}

static int DockHostWindow_GetZoneTabLengthFromLabel(HDC hdc, PCWSTR pszLabel)
{
	if (!hdc || !pszLabel || !pszLabel[0])
	{
		return DOCKLAYOUT_ZONE_TAB_LENGTH;
	}

	int cch = (int)wcslen(pszLabel);
	SIZE size = { 0 };
	if (!GetTextExtentPoint32(hdc, pszLabel, cch, &size))
	{
		return DOCKLAYOUT_ZONE_TAB_LENGTH;
	}

	int iTabLength = max(size.cx + 16, DOCKLAYOUT_ZONE_TAB_THICKNESS * 2);
	iTabLength = min(iTabLength, 280);
	return iTabLength;
}

static BOOL DockHostWindow_GetZoneTabRect(DockHostWindow* pDockHostWindow, int nDockSide, int iOffset, int iTabLength, RECT* pRect)
{
	if (!pDockHostWindow || !pRect)
	{
		return FALSE;
	}

	RECT rcClient = { 0 };
	GetClientRect(Window_GetHWND((Window*)pDockHostWindow), &rcClient);
	return DockLayout_GetZoneTabRectByOffset(&rcClient, nDockSide, iOffset, iTabLength, pRect);
}

static int DockHostWindow_HitTestZoneTab(DockHostWindow* pDockHostWindow, int x, int y, HWND* phWndTab)
{
	POINT pt = { x, y };
	if (phWndTab)
	{
		*phWndTab = NULL;
	}

	HWND hWndHost = Window_GetHWND((Window*)pDockHostWindow);
	HDC hdc = hWndHost ? GetDC(hWndHost) : NULL;
	HFONT hFontPrev = NULL;
	if (hdc)
	{
		hFontPrev = (HFONT)SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));
	}

	const int sides[] = { DKS_LEFT, DKS_RIGHT, DKS_TOP, DKS_BOTTOM };
	for (int i = 0; i < ARRAYSIZE(sides); ++i)
	{
		TreeNode* pZone = DockHostWindow_GetZoneNode(pDockHostWindow, sides[i]);
		if (!pZone)
		{
			continue;
		}

		TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
		int nTabs = DockZone_GetPanelsByCollapsed(pZone, tabs, ARRAYSIZE(tabs), TRUE);
		if (nTabs <= 0)
		{
			continue;
		}

		int iOffset = 0;
		for (int iTab = 0; iTab < nTabs; ++iTab)
		{
			DockData* pDockDataTab = tabs[iTab] ? (DockData*)tabs[iTab]->data : NULL;
			WCHAR szLabel[64] = L"";
			DockHostWindow_GetPanelTabLabel(pDockDataTab, szLabel, ARRAYSIZE(szLabel));
			int iTabLength = DockHostWindow_GetZoneTabLengthFromLabel(hdc, szLabel);

			RECT rcTab = { 0 };
			if (!DockHostWindow_GetZoneTabRect(pDockHostWindow, sides[i], iOffset, iTabLength, &rcTab))
			{
				iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;
				continue;
			}
			iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;

			if (PtInRect(&rcTab, pt))
			{
				if (phWndTab && tabs[iTab] && tabs[iTab]->data)
				{
					*phWndTab = ((DockData*)tabs[iTab]->data)->hWnd;
				}

				if (hdc)
				{
					SelectObject(hdc, hFontPrev);
					ReleaseDC(hWndHost, hdc);
				}
				return sides[i];
			}
		}
	}

	if (hdc)
	{
		SelectObject(hdc, hFontPrev);
		ReleaseDC(hWndHost, hdc);
	}

	return DKS_NONE;
}

static void DockHostWindow_DrawZoneTabs(DockHostWindow* pDockHostWindow, HDC hdc)
{
	HFONT hUIFont = PanitentApp_GetUIFont(PanitentApp_Instance());
	HFONT hFontPrev = (HFONT)SelectObject(hdc, hUIFont);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, COLORREF_WHITE);

	const int sides[] = { DKS_LEFT, DKS_RIGHT, DKS_TOP, DKS_BOTTOM };
	for (int i = 0; i < ARRAYSIZE(sides); ++i)
	{
		TreeNode* pZone = DockHostWindow_GetZoneNode(pDockHostWindow, sides[i]);
		if (!pZone)
		{
			continue;
		}

		TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
		int nTabs = DockZone_GetPanelsByCollapsed(pZone, tabs, ARRAYSIZE(tabs), TRUE);
		if (nTabs <= 0)
		{
			continue;
		}

		int iOffset = 0;
		for (int iTab = 0; iTab < nTabs; ++iTab)
		{
			DockData* pDockDataTab = tabs[iTab] ? (DockData*)tabs[iTab]->data : NULL;
			HWND hWndTab = pDockDataTab ? pDockDataTab->hWnd : NULL;
			WCHAR szLabel[64] = L"";
			DockHostWindow_GetPanelTabLabel(pDockDataTab, szLabel, ARRAYSIZE(szLabel));
			int iTabLength = DockHostWindow_GetZoneTabLengthFromLabel(hdc, szLabel);

			RECT rcTab = { 0 };
			if (!DockHostWindow_GetZoneTabRect(pDockHostWindow, sides[i], iOffset, iTabLength, &rcTab))
			{
				iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;
				continue;
			}
			iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;

			BOOL bActive = pDockHostWindow->fAutoHideOverlayVisible &&
				pDockHostWindow->nAutoHideOverlaySide == sides[i] &&
				hWndTab &&
				hWndTab == pDockHostWindow->hWndAutoHideOverlay;

			SelectObject(hdc, GetStockObject(DC_BRUSH));
			SelectObject(hdc, GetStockObject(DC_PEN));
			{
				PanitentThemeColors colors = { 0 };
				PanitentTheme_GetColors(&colors);
				SetDCBrushColor(hdc, bActive ? colors.accentActive : colors.accent);
				SetDCPenColor(hdc, colors.border);
			}
			Rectangle(hdc, rcTab.left, rcTab.top, rcTab.right, rcTab.bottom);

			int cchLabel = (int)wcslen(szLabel);
			SIZE textSize = { 0 };
			if (!GetTextExtentPoint32(hdc, szLabel, cchLabel, &textSize))
			{
				textSize.cx = 0;
				textSize.cy = 0;
			}

			if (sides[i] == DKS_LEFT || sides[i] == DKS_RIGHT)
			{
				LOGFONT lf = { 0 };
				GetObject(hUIFont, sizeof(lf), &lf);
				lf.lfEscapement = (sides[i] == DKS_LEFT) ? 900 : 2700;
				lf.lfOrientation = lf.lfEscapement;
				HFONT hVertical = CreateFontIndirect(&lf);
				HFONT hOldVertical = (HFONT)SelectObject(hdc, hVertical);
				UINT uPrevAlign = SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

				int iTabThickness = Win32_Rect_GetWidth(&rcTab);
				int iTabSpan = Win32_Rect_GetHeight(&rcTab);
				int tx = max(rcTab.left, rcTab.left + (iTabThickness - textSize.cy) / 2);
				int ty = max(rcTab.top, rcTab.top + (iTabSpan - textSize.cx) / 2);
				if (sides[i] == DKS_LEFT)
				{
					ty = min(rcTab.bottom, rcTab.top + (iTabSpan + textSize.cx) / 2);
				}
				else {
					tx = min(rcTab.right, rcTab.right - (iTabThickness - textSize.cy) / 2);
				}

				TextOut(hdc, tx, ty, szLabel, (int)wcsnlen_s(szLabel, ARRAYSIZE(szLabel)));
				SetTextAlign(hdc, uPrevAlign);
				SelectObject(hdc, hOldVertical);
				DeleteObject(hVertical);
			}
			else {
				DrawText(hdc, szLabel, -1, &rcTab, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			}
		}
	}

	SelectObject(hdc, hFontPrev);
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

static BOOL DockHostWindow_IsSplitVertical(TreeNode* pNode)
{
	if (!pNode || !pNode->data)
	{
		return FALSE;
	}

	return ((((DockData*)pNode->data)->dwStyle & DGD_VERTICAL) != 0) ? TRUE : FALSE;
}

static BOOL DockHostWindow_GetSplitRect(TreeNode* pNode, RECT* pRect)
{
	if (!pNode || !pRect || !pNode->data || !pNode->node1 || !pNode->node2)
	{
		return FALSE;
	}

	DockData* pDockData = (DockData*)pNode->data;
	if (pDockData->dwStyle & DGP_RELATIVE)
	{
		return FALSE;
	}

	if (!DockNode_HasVisibleWindow(pNode->node1) || !DockNode_HasVisibleWindow(pNode->node2))
	{
		return FALSE;
	}

	RECT rcClient = { 0 };
	if (!DockData_GetClientRect(pDockData, &rcClient))
	{
		return FALSE;
	}

	BOOL bVertical = DockHostWindow_IsSplitVertical(pNode);
	int iSpan = bVertical ? Win32_Rect_GetHeight(&rcClient) : Win32_Rect_GetWidth(&rcClient);
	if (iSpan <= 0)
	{
		return FALSE;
	}

	int iGrip = DockLayout_ClampSplitGrip(iSpan, pDockData->iGripPos, 0);
	int iPos = 0;
	if (pDockData->dwStyle & DGA_END)
	{
		iPos = bVertical ? (rcClient.bottom - iGrip) : (rcClient.right - iGrip);
	}
	else {
		iPos = bVertical ? (rcClient.top + iGrip) : (rcClient.left + iGrip);
	}

	int iThickness = max(iBorderWidth, 4);
	int iHalf = iThickness / 2;

	RECT rcSplit = rcClient;
	if (bVertical)
	{
		rcSplit.top = iPos - iHalf;
		rcSplit.bottom = rcSplit.top + iThickness;
	}
	else {
		rcSplit.left = iPos - iHalf;
		rcSplit.right = rcSplit.left + iThickness;
	}

	if (!IntersectRect(pRect, &rcSplit, &rcClient))
	{
		return FALSE;
	}

	return Win32_Rect_GetWidth(pRect) > 0 && Win32_Rect_GetHeight(pRect) > 0;
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
		if (!DockHostWindow_GetSplitRect(pCurrentNode, &rcSplit))
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

	BOOL bVertical = DockHostWindow_IsSplitVertical(pDockHostWindow->pSplitNode);
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

static void DockHostWindow_DrawSplitGrips(DockHostWindow* pDockHostWindow, HDC hdc)
{
	if (!pDockHostWindow || !pDockHostWindow->pRoot_)
	{
		return;
	}

	TreeTraversalRLOT traversal = { 0 };
	TreeTraversalRLOT_Init(&traversal, pDockHostWindow->pRoot_);

	TreeNode* pCurrentNode = NULL;
	while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
	{
		RECT rcSplit = { 0 };
		if (!DockHostWindow_GetSplitRect(pCurrentNode, &rcSplit))
		{
			continue;
		}

		BOOL bVertical = DockHostWindow_IsSplitVertical(pCurrentNode);
		SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(DC_PEN));
		{
			PanitentThemeColors colors = { 0 };
			PanitentTheme_GetColors(&colors);
			SetDCBrushColor(hdc, colors.splitterFill);
			SetDCPenColor(hdc, colors.border);
		}
		Rectangle(hdc, rcSplit.left, rcSplit.top, rcSplit.right, rcSplit.bottom);

		{
			PanitentThemeColors colors = { 0 };
			PanitentTheme_GetColors(&colors);
			SetDCBrushColor(hdc, colors.splitterGrip);
		}
		if (bVertical)
		{
			int cx = (rcSplit.left + rcSplit.right) / 2;
			int cy = (rcSplit.top + rcSplit.bottom) / 2;
			for (int i = -1; i <= 1; ++i)
			{
				Rectangle(hdc, cx - 2 + i * 8, cy - 1, cx + 2 + i * 8, cy + 1);
			}
		}
		else {
			int cx = (rcSplit.left + rcSplit.right) / 2;
			int cy = (rcSplit.top + rcSplit.bottom) / 2;
			for (int i = -1; i <= 1; ++i)
			{
				Rectangle(hdc, cx - 1, cy - 2 + i * 8, cx + 1, cy + 2 + i * 8);
			}
		}
	}

	TreeTraversalRLOT_Destroy(&traversal);
}

void DockNode_Paint(DockHostWindow* pDockHostWindow, TreeNode* pNodeParent, HDC hdc, HBRUSH hCaptionBrush)
{
	UNREFERENCED_PARAMETER(hCaptionBrush);

	/* Break if node is invalid */
	if (!pNodeParent)
	{
		return;
	}

	TreeTraversalRLOT dockNodeTraversal;
	TreeTraversalRLOT_Init(&dockNodeTraversal, pNodeParent);

	TreeNode* pCurrentNode = NULL;
	while (pCurrentNode = TreeTraversalRLOT_GetNext(&dockNodeTraversal))
	{
		DockData* pDockData = (DockData*)pCurrentNode->data;

		if (!pDockData)
		{
			continue;
		}

		RECT* rc = &pDockData->rc;
	
		if (pDockData->bShowCaption)
		{
			CaptionFrameLayout layout = { 0 };
			if (Dock_BuildCaptionLayout(pDockData, FALSE, &layout))
			{
				CaptionFramePalette palette = { 0 };
				PanitentTheme_GetCaptionPalette(&palette);

				int nHotButton = DCB_NONE;
				int nPressedButton = DCB_NONE;
				if (pDockHostWindow)
				{
					if (pDockHostWindow->pCaptionHotNode == pCurrentNode)
					{
						nHotButton = pDockHostWindow->nCaptionHotButton;
					}
					if (pDockHostWindow->pCaptionPressedNode == pCurrentNode)
					{
						nPressedButton = pDockHostWindow->nCaptionPressedButton;
					}
				}

				HFONT guiFont = PanitentApp_GetUIFont(PanitentApp_Instance());
				CaptionFrame_DrawStateful(hdc, &layout, &palette, pDockData->lpszCaption, guiFont, nHotButton, nPressedButton);
			}
	
			/* ================================================================================ */
	
			/* Redraw window */
	
			if (pCurrentNode->data && ((DockData*)pCurrentNode->data)->hWnd)
			{
				HWND hWnd = ((DockData*)pCurrentNode->data)->hWnd;
				InvalidateRect(hWnd, NULL, FALSE);
			}
		}
	}
	
	TreeTraversalRLOT_Destroy(&dockNodeTraversal);
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

#define DOCKHOSTBGMARGIN 16

static BOOL DockHostWindow_EnsureWatermarkCache(HDC hdc, int idBitmap, COLORREF tint)
{
	HDC hdcMask = NULL;
	HBITMAP hOldBitmap = NULL;
	BITMAP bm = { 0 };
	BITMAPINFO bmi = { 0 };
	uint32_t* pPixels = NULL;
	HDC hdcColored = NULL;
	HBITMAP hOldColored = NULL;
	BYTE targetR = GetRValue(tint);
	BYTE targetG = GetGValue(tint);
	BYTE targetB = GetBValue(tint);

	if (!hdc)
	{
		return FALSE;
	}

	if (!g_dockHostWatermarkCache.hSourceBitmap)
	{
		g_dockHostWatermarkCache.hSourceBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(idBitmap));
		if (!g_dockHostWatermarkCache.hSourceBitmap)
		{
			return FALSE;
		}

		if (!GetObject(g_dockHostWatermarkCache.hSourceBitmap, sizeof(BITMAP), &bm))
		{
			DeleteObject(g_dockHostWatermarkCache.hSourceBitmap);
			g_dockHostWatermarkCache.hSourceBitmap = NULL;
			return FALSE;
		}

		g_dockHostWatermarkCache.width = bm.bmWidth;
		g_dockHostWatermarkCache.height = bm.bmHeight;
	}

	if (g_dockHostWatermarkCache.hTintedBitmap &&
		g_dockHostWatermarkCache.tint == tint)
	{
		return TRUE;
	}

	if (g_dockHostWatermarkCache.hTintedBitmap)
	{
		DeleteObject(g_dockHostWatermarkCache.hTintedBitmap);
		g_dockHostWatermarkCache.hTintedBitmap = NULL;
	}

	if (g_dockHostWatermarkCache.width <= 0 || g_dockHostWatermarkCache.height <= 0)
	{
		return FALSE;
	}

	hdcMask = CreateCompatibleDC(hdc);
	if (!hdcMask)
	{
		return FALSE;
	}

	hOldBitmap = (HBITMAP)SelectObject(hdcMask, g_dockHostWatermarkCache.hSourceBitmap);
	if (!hOldBitmap)
	{
		DeleteDC(hdcMask);
		return FALSE;
	}

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = g_dockHostWatermarkCache.width;
	bmi.bmiHeader.biHeight = -g_dockHostWatermarkCache.height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	g_dockHostWatermarkCache.hTintedBitmap = CreateDIBSection(
		hdc,
		&bmi,
		DIB_RGB_COLORS,
		(LPVOID*)&pPixels,
		NULL,
		0);
	if (!g_dockHostWatermarkCache.hTintedBitmap || !pPixels)
	{
		g_dockHostWatermarkCache.hTintedBitmap = NULL;
		SelectObject(hdcMask, hOldBitmap);
		DeleteDC(hdcMask);
		return FALSE;
	}

	hdcColored = CreateCompatibleDC(hdc);
	if (!hdcColored)
	{
		DeleteObject(g_dockHostWatermarkCache.hTintedBitmap);
		g_dockHostWatermarkCache.hTintedBitmap = NULL;
		SelectObject(hdcMask, hOldBitmap);
		DeleteDC(hdcMask);
		return FALSE;
	}

	hOldColored = (HBITMAP)SelectObject(hdcColored, g_dockHostWatermarkCache.hTintedBitmap);
	if (!hOldColored)
	{
		DeleteDC(hdcColored);
		DeleteObject(g_dockHostWatermarkCache.hTintedBitmap);
		g_dockHostWatermarkCache.hTintedBitmap = NULL;
		SelectObject(hdcMask, hOldBitmap);
		DeleteDC(hdcMask);
		return FALSE;
	}

	for (int y = 0; y < g_dockHostWatermarkCache.height; ++y)
	{
		for (int x = 0; x < g_dockHostWatermarkCache.width; ++x)
		{
			COLORREF srcColor = GetPixel(hdcMask, x, y);
			BYTE mask = (BYTE)(((int)GetRValue(srcColor) + (int)GetGValue(srcColor) + (int)GetBValue(srcColor)) / 3);
			BYTE outR = (BYTE)(((int)targetR * (int)mask + 127) / 255);
			BYTE outG = (BYTE)(((int)targetG * (int)mask + 127) / 255);
			BYTE outB = (BYTE)(((int)targetB * (int)mask + 127) / 255);

			pPixels[(size_t)y * (size_t)g_dockHostWatermarkCache.width + (size_t)x] =
				((uint32_t)mask << 24) |
				((uint32_t)outR << 16) |
				((uint32_t)outG << 8) |
				(uint32_t)outB;
		}
	}

	g_dockHostWatermarkCache.tint = tint;

	SelectObject(hdcColored, hOldColored);
	DeleteDC(hdcColored);
	SelectObject(hdcMask, hOldBitmap);
	DeleteDC(hdcMask);
	return TRUE;
}

static BOOL DockHostWindow_DrawMaskedBitmap(HDC hdc, const RECT* pDestRect, int idBitmap, COLORREF tint)
{
	HDC hdcColored = NULL;
	HBITMAP hOldColored = NULL;
	int destWidth = 0;
	int destHeight = 0;
	BOOL fDrawn = FALSE;

	if (!hdc || !pDestRect)
	{
		return FALSE;
	}

	destWidth = pDestRect->right - pDestRect->left;
	destHeight = pDestRect->bottom - pDestRect->top;
	if (destWidth <= 0 || destHeight <= 0)
	{
		return FALSE;
	}

	if (!DockHostWindow_EnsureWatermarkCache(hdc, idBitmap, tint))
	{
		return FALSE;
	}

	hdcColored = CreateCompatibleDC(hdc);
	if (!hdcColored)
	{
		return FALSE;
	}

	hOldColored = (HBITMAP)SelectObject(hdcColored, g_dockHostWatermarkCache.hTintedBitmap);
	if (!hOldColored)
	{
		DeleteDC(hdcColored);
		return FALSE;
	}

	{
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 0xFF;
		blend.AlphaFormat = AC_SRC_ALPHA;
		fDrawn = AlphaBlend(
			hdc,
			pDestRect->left,
			pDestRect->top,
			destWidth,
			destHeight,
			hdcColored,
			0,
			0,
			g_dockHostWatermarkCache.width,
			g_dockHostWatermarkCache.height,
			blend);
	}

	SelectObject(hdcColored, hOldColored);
	DeleteDC(hdcColored);
	return fDrawn;
}

static void DockHostWindow_PaintContent(DockHostWindow* pDockHostWindow, HDC hdc, const RECT* pClientRect)
{
	SelectObject(hdc, GetStockObject(DC_BRUSH));
	{
		PanitentThemeColors colors = { 0 };
		PanitentTheme_GetColors(&colors);
		SetDCBrushColor(hdc, colors.rootBackground);
	}
	FillRect(hdc, pClientRect, (HBRUSH)GetStockObject(DC_BRUSH));

	if (pDockHostWindow->pRoot_) {
		DockHostWindow_SyncZones(pDockHostWindow);
		DockNode_Paint(pDockHostWindow, pDockHostWindow->pRoot_, hdc, pDockHostWindow->hCaptionBrush_);
		DockHostWindow_DrawSplitGrips(pDockHostWindow, hdc);
		DockHostWindow_DrawZoneTabs(pDockHostWindow, hdc);
	}
	else {
		HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCKHOSTBG));
		if (hBitmap)
		{
			BITMAP bm = { 0 };
			GetObject(hBitmap, sizeof(BITMAP), &bm);
			if (bm.bmWidth > 0 && bm.bmHeight > 0)
			{
				PanitentThemeColors colors = { 0 };
				RECT rcLogo = { 0 };
				int clientWidth = pClientRect->right - pClientRect->left;
				int clientHeight = pClientRect->bottom - pClientRect->top;

				PanitentTheme_GetColors(&colors);
				SetRect(
					&rcLogo,
					clientWidth - bm.bmWidth - DOCKHOSTBGMARGIN,
					clientHeight - bm.bmHeight - DOCKHOSTBGMARGIN,
					clientWidth - DOCKHOSTBGMARGIN,
					clientHeight - DOCKHOSTBGMARGIN);
				DockHostWindow_DrawMaskedBitmap(hdc, &rcLogo, IDB_DOCKHOSTBG, colors.accentActive);
			}
			DeleteObject(hBitmap);
		}
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

	DockHostWindow_PaintContent(pDockHostWindow, hdcTarget, &rcClient);

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

HWND g_hWndDragOverlay;

static void DockHostWindow_DestroyDragOverlay(void)
{
	if (g_hWndDragOverlay && IsWindow(g_hWndDragOverlay))
	{
		DestroyWindow(g_hWndDragOverlay);
	}

	g_hWndDragOverlay = NULL;
}

static void DockHostWindow_ContinueFloatingDrag(HWND hWndFloating)
{
	if (!hWndFloating || !IsWindow(hWndFloating))
	{
		return;
	}

	POINT ptCursor = { 0 };
	GetCursorPos(&ptCursor);

	ReleaseCapture();
	SetForegroundWindow(hWndFloating);
	SetActiveWindow(hWndFloating);

	/* Continue drag with system non-client move loop. */
	LPARAM lParam = MAKELPARAM((short)ptCursor.x, (short)ptCursor.y);
	SendMessage(hWndFloating, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
}

void DockHostWindow_UndockToFloating(DockHostWindow* pDockHostWindow, TreeNode* pNode, int x, int y)
{
	if (!pDockHostWindow || !pNode || !pNode->data)
	{
		return;
	}

	DockData* pDockData = (DockData*)pNode->data;
	RECT rcDockLocal = pDockData->rc;
	int dockWidth = max(Win32_Rect_GetWidth(&rcDockLocal), 1);
	int dockHeight = max(Win32_Rect_GetHeight(&rcDockLocal), 1);

	int dragOffsetX = x - rcDockLocal.left;
	int dragOffsetY = y - rcDockLocal.top;
	dragOffsetX = max(0, min(dragOffsetX, dockWidth - 1));
	dragOffsetY = max(0, min(dragOffsetY, dockHeight - 1));

	POINT ptCursor = { 0 };
	GetCursorPos(&ptCursor);

	DockHostWindow_Undock(pDockHostWindow, pNode);

	FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
	HWND hWndFloating = Window_CreateWindow((Window*)pFloatingWindowContainer, NULL);
	FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
	FloatingWindowContainer_PinWindow(pFloatingWindowContainer, pDockData->hWnd);

	RECT rcFloating = rcDockLocal;
	MapWindowPoints(Window_GetHWND((Window*)pDockHostWindow), HWND_DESKTOP, (POINT*)&rcFloating, 2);

	int width = max(Win32_Rect_GetWidth(&rcFloating), 260);
	int height = max(Win32_Rect_GetHeight(&rcFloating), 220);
	int floatingX = ptCursor.x - dragOffsetX;
	int floatingY = ptCursor.y - dragOffsetY;
	SetWindowPos(hWndFloating, NULL, floatingX, floatingY, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

	DockHostWindow_Rearrange(pDockHostWindow);
	DockHostWindow_DestroyDragOverlay();
	DockHostWindow_ContinueFloatingDrag(hWndFloating);
}

void DockHostWindow_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags)
{
	UNREFERENCED_PARAMETER(keyFlags);
	TRACKMOUSEEVENT tme = { 0 };
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = Window_GetHWND((Window*)pDockHostWindow);
	TrackMouseEvent(&tme);

	if (pDockHostWindow->fSplitDrag)
	{
		DockHostWindow_ClearCaptionButtonState(pDockHostWindow);
		DockHostWindow_UpdateSplitDrag(pDockHostWindow, x, y);
		return;
	}

	if (pDockHostWindow->fCaptionDrag)
	{
		DockHostWindow_ClearCaptionButtonState(pDockHostWindow);
		int distance = (int)roundf(sqrtf((powf(pDockHostWindow->ptDragPos_.x - x, 2.0f) + powf(pDockHostWindow->ptDragPos_.y - y, 2.0f))));
		int activateDistance = DRAG_UNDOCK_DISTANCE;
		DockHostWindow_UpdateDragOverlayVisual(pDockHostWindow, distance);

		if (distance >= activateDistance)
		{
			pDockHostWindow->fCaptionDrag = FALSE;
			DockHostWindow_UndockToFloating(pDockHostWindow, pDockHostWindow->m_pSubjectNode, x, y);
		}
		/*
		else {
			POINT pt = { 0 };
			pt.x = pDockHostWindow->ptDragPos_.x;
			pt.y = pDockHostWindow->ptDragPos_.y;
			HDC hScreenDC = GetDC(NULL);
			ClientToScreen(pDockHostWindow->base.hWnd, &pt);

			Ellipse(hScreenDC, pt.x - activateDistance, pt.y - activateDistance, pt.x + activateDistance, pt.y + activateDistance);
		}
		*/
		return;
	}

	TreeNode* pHitNode = NULL;
	int nHitType = DockHostWindow_HitTest(pDockHostWindow, &pHitNode, x, y);
	DockHostWindow_SetCaptionHotButton(pDockHostWindow, pHitNode, Dock_HitTypeToCaptionButton(nHitType));

	TreeNode* pSplitNode = DockHostWindow_HitTestSplitGrip(pDockHostWindow, x, y);
	if (pSplitNode && pSplitNode->data)
	{
		if (DockHostWindow_IsSplitVertical(pSplitNode))
		{
			SetCursor(LoadCursor(NULL, IDC_SIZENS));
		}
		else {
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		}
	}
}

LRESULT CALLBACK DragOverlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static float smoothstepf(float edge0, float edge1, float x)
{
	float t = fminf(fmaxf((x - edge0) / (edge1 - edge0), 0.0f), 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

static float clampf(float value, float minValue, float maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}
	else if (value > maxValue)
	{
		return maxValue;
	}
	else {
		return value;
	}
}

#define DRAG_OVERLAY_SIZE 128
#define DRAG_OVERLAY_CENTER (DRAG_OVERLAY_SIZE / 2)

static void DockHostWindow_UpdateDragOverlayVisual(DockHostWindow* pDockHostWindow, int iRadius)
{
	if (!pDockHostWindow || !g_hWndDragOverlay || !IsWindow(g_hWndDragOverlay))
	{
		return;
	}

	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = DRAG_OVERLAY_SIZE;
	bmi.bmiHeader.biHeight = -DRAG_OVERLAY_SIZE;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	unsigned int* pBits = NULL;
	HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);
	memset(pBits, 0, DRAG_OVERLAY_SIZE * DRAG_OVERLAY_SIZE * sizeof(unsigned int));

	float radius = clampf((float)iRadius, 0.0f, (float)(DRAG_OVERLAY_CENTER - 6));
	float readiness = clampf(radius / (float)DRAG_UNDOCK_DISTANCE, 0.0f, 1.0f);
	float heat = smoothstepf(0.72f, 1.0f, readiness);
	float ringHalfWidth = 2.2f + heat * 0.8f;
	float ringFeather = 2.4f + heat * 1.2f;

	const float coldR = 0x62;
	const float coldG = 0xC6;
	const float coldB = 0xFF;
	const float hotR = 0xFF;
	const float hotG = 0x8A;
	const float hotB = 0x4A;
	float srcR = coldR + (hotR - coldR) * heat;
	float srcG = coldG + (hotG - coldG) * heat;
	float srcB = coldB + (hotB - coldB) * heat;

	for (int y = 0; y < DRAG_OVERLAY_SIZE; ++y)
	{
		for (int x = 0; x < DRAG_OVERLAY_SIZE; ++x)
		{
			float dx = (float)x + 0.5f - (float)DRAG_OVERLAY_CENTER;
			float dy = (float)y + 0.5f - (float)DRAG_OVERLAY_CENTER;
			float distance = sqrtf(dx * dx + dy * dy);
			float ringDistance = fabsf(distance - radius);
			float core = 1.0f - smoothstepf(ringHalfWidth, ringHalfWidth + ringFeather, ringDistance);
			float glow = (1.0f - smoothstepf(ringHalfWidth + 1.5f, ringHalfWidth + 8.5f, ringDistance)) * 0.38f;
			float alphaNorm = clampf(core + glow, 0.0f, 1.0f);
			if (radius < 1.0f)
			{
				alphaNorm = 0.0f;
			}

			float alphaBoost = 220.0f + heat * 20.0f;
			BYTE alpha = (BYTE)clampf(alphaBoost * alphaNorm, 0.0f, 255.0f);
			BYTE red = (BYTE)clampf((srcR * alpha) / 255.0f, 0.0f, 255.0f);
			BYTE green = (BYTE)clampf((srcG * alpha) / 255.0f, 0.0f, 255.0f);
			BYTE blue = (BYTE)clampf((srcB * alpha) / 255.0f, 0.0f, 255.0f);
			pBits[y * DRAG_OVERLAY_SIZE + x] = ((unsigned int)alpha << 24) | ((unsigned int)red << 16) | ((unsigned int)green << 8) | (unsigned int)blue;
		}
	}

	POINT ptPos = { pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y };
	ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptPos);
	ptPos.x -= DRAG_OVERLAY_CENTER;
	ptPos.y -= DRAG_OVERLAY_CENTER;

	SIZE sizeWnd = { DRAG_OVERLAY_SIZE, DRAG_OVERLAY_SIZE };
	POINT ptSrc = { 0, 0 };
	BLENDFUNCTION blendFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	UpdateLayeredWindow(g_hWndDragOverlay, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blendFunction, ULW_ALPHA);

	SelectObject(hdcMem, hOldObj);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
}

void DockHostWindow_StartDrag(DockHostWindow* pDockHostWindow, int x, int y)
{
	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);

	WNDCLASSEX wcex = { 0 };
	if (!GetClassInfoEx(GetModuleHandle(NULL), L"__DragOverlayClass", &wcex))
	{
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = (WNDPROC)DragOverlayWndProc;
		wcex.hInstance = GetModuleHandle(NULL);
		wcex.lpszClassName = L"__DragOverlayClass";
		RegisterClassEx(&wcex);
	}

	POINT ptStart = { pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y };
	ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptStart);

	DockHostWindow_DestroyDragOverlay();
	g_hWndDragOverlay = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
		L"__DragOverlayClass",
		L"DragOverlay",
		WS_VISIBLE | WS_POPUP,
		ptStart.x - DRAG_OVERLAY_CENTER,
		ptStart.y - DRAG_OVERLAY_CENTER,
		DRAG_OVERLAY_SIZE,
		DRAG_OVERLAY_SIZE,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL);

	DockHostWindow_UpdateDragOverlayVisual(pDockHostWindow, 0);
}

void DockHostWindow_OnLButtonDown(DockHostWindow* pDockHostWindow, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	UNREFERENCED_PARAMETER(fDoubleClick);
	UNREFERENCED_PARAMETER(keyFlags);

	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);

	/* Get whole dock window client rect */
	RECT rcClient = { 0 };
	GetClientRect(hWnd, &rcClient);

	if (pDockHostWindow->fAutoHideOverlayVisible)
	{
		POINT pt = { x, y };
		if (PtInRect(&pDockHostWindow->rcAutoHideOverlay, pt))
		{
			return;
		}
	}

	HWND hWndZoneTab = NULL;
	int zoneSide = DockHostWindow_HitTestZoneTab(pDockHostWindow, x, y, &hWndZoneTab);
	if (zoneSide != DKS_NONE)
	{
		DockHostWindow_ToggleZoneCollapsed(pDockHostWindow, zoneSide, hWndZoneTab);
		return;
	}

	if (pDockHostWindow->fAutoHideOverlayVisible)
	{
		POINT pt = { x, y };
		if (!PtInRect(&pDockHostWindow->rcAutoHideOverlay, pt))
		{
			DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
			Window_Invalidate((Window*)pDockHostWindow);
		}
	}

	TreeNode* pSplitNode = DockHostWindow_HitTestSplitGrip(pDockHostWindow, x, y);
	if (pSplitNode)
	{
		DockHostWindow_BeginSplitDrag(pDockHostWindow, pSplitNode, x, y);
		return;
	}

	TreeNode* pTreeNode = NULL;
	int htType = DockHostWindow_HitTest(pDockHostWindow, &pTreeNode, x, y);
	int nHitButton = Dock_HitTypeToCaptionButton(htType);

	switch (htType)
	{
	case DHT_CLOSEBTN:
	case DHT_PINBTN:
	case DHT_MOREBTN:
	{
		DockHostWindow_SetCaptionHotButton(pDockHostWindow, pTreeNode, nHitButton);
		DockHostWindow_SetCaptionPressedButton(pDockHostWindow, pTreeNode, nHitButton);
		SetCapture(hWnd);
		return;
	}
	break;

	case DHT_CAPTION:
	{
		DockHostWindow_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
		pDockHostWindow->fCaptionDrag = TRUE;
		pDockHostWindow->m_pSubjectNode = pTreeNode;

		SetCapture(hWnd);

		/* Save click position */
		pDockHostWindow->ptDragPos_.x = x - (rcClient.left + 2);
		pDockHostWindow->ptDragPos_.y = y - (rcClient.top + 2);
		pDockHostWindow->fDrag_ = TRUE;

		DockHostWindow_StartDrag(pDockHostWindow, pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y);
	}
	break;

	default:
		DockHostWindow_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
		break;
	}
}

void DockHostWindow_InvokeDockInspectorDialog(DockHostWindow* pDockHostWindow)
{
	INT_PTR hDialog = Dialog_CreateWindow((Dialog*)pDockHostWindow->m_pDockInspectorDialog, IDD_DOCKINSPECTOR, Window_GetHWND((Window*)pDockHostWindow), FALSE);
	HWND hWndDialog = (HWND)hDialog;
	if (hWndDialog && IsWindow(hWndDialog))
	{
		/* Important. Idk why */
		ShowWindow(hWndDialog, SW_SHOW);
	}
}

void DockHostWindow_OnLButtonUp(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags) {
	UNREFERENCED_PARAMETER(keyFlags);

	HWND hWndDockHost = Window_GetHWND((Window*)pDockHostWindow);

	if (pDockHostWindow->fSplitDrag)
	{
		DockHostWindow_EndSplitDrag(pDockHostWindow);
		return;
	}

	if (pDockHostWindow->nCaptionPressedButton != DCB_NONE)
	{
		TreeNode* pHitNode = NULL;
		int htType = DockHostWindow_HitTest(pDockHostWindow, &pHitNode, x, y);
		int nHitButton = Dock_HitTypeToCaptionButton(htType);

		TreeNode* pPressedNode = pDockHostWindow->pCaptionPressedNode;
		int nPressedButton = pDockHostWindow->nCaptionPressedButton;
		DockHostWindow_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
		DockHostWindow_SetCaptionHotButton(pDockHostWindow, pHitNode, nHitButton);

		if (GetCapture() == hWndDockHost)
		{
			ReleaseCapture();
		}

		if (pHitNode != pPressedNode || nHitButton != nPressedButton)
		{
			pDockHostWindow->fDrag_ = FALSE;
			DockHostWindow_DestroyDragOverlay();
			return;
		}

		switch (nPressedButton)
		{
		case DCB_CLOSE:
			DockHostWindow_DestroyInclusive(pDockHostWindow, pPressedNode);
			Window_Invalidate((Window*)pDockHostWindow);
			return;

		case DCB_PIN:
			DockHostWindow_TogglePanelPinned(pDockHostWindow, pPressedNode);
			return;

		case DCB_MORE:
		{
			POINT ptScreen = { x, y };
			ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptScreen);
			DockHostWindow_ShowPanelMenu(pDockHostWindow, pPressedNode, ptScreen);
			return;
		}
		}
	}

	TreeNode* pTreeNode = NULL;
	int htType = DockHostWindow_HitTest(pDockHostWindow, &pTreeNode, x, y);

	switch (htType)
	{
	case DHT_CLOSEBTN:
	{
		DockHostWindow_DestroyInclusive(pDockHostWindow, pTreeNode);
		Window_Invalidate((Window*)pDockHostWindow);
	}
	break;

	case DHT_PINBTN:
	{
		DockHostWindow_TogglePanelPinned(pDockHostWindow, pTreeNode);
	}
	break;

	case DHT_MOREBTN:
	{
		POINT ptScreen = { x, y };
		ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptScreen);
		DockHostWindow_ShowPanelMenu(pDockHostWindow, pTreeNode, ptScreen);
	}
	break;

	case DHT_CAPTION:
	{
		/* Do nothing. */
	}
	break;
	}

	pDockHostWindow->fDrag_ = FALSE;
	DockHostWindow_DestroyDragOverlay();
	if (GetCapture() == hWndDockHost)
	{
		ReleaseCapture();
	}
}

#define IDM_DOCKINSPECTOR 101

void DockHostWindow_OnContextMenu(DockHostWindow* pDockHostWindow, HWND hWndContext, int x, int y)
{
	UNREFERENCED_PARAMETER(hWndContext);

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	ScreenToClient(Window_GetHWND((Window*)pDockHostWindow), &pt);

	TreeNode* pTreeNode = NULL;
	int htType = DockHostWindow_HitTest(pDockHostWindow, &pTreeNode, pt.x, pt.y);

	switch (htType)
	{
	case DHT_CLOSEBTN:
	case DHT_PINBTN:
	case DHT_MOREBTN:
			{
				/* Do nothing. */
			}
			break;

	case DHT_CAPTION:
		{
			/* Do nothing. */
		}
		break;

	case DHT_UNKNOWN:
		{
			HMENU hMenu = CreatePopupMenu();
			if (hMenu)
			{
				InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_DOCKINSPECTOR, L"Inspect Element...");
				TrackPopupMenu(hMenu, 0, x, y, 0, Window_GetHWND((Window*)pDockHostWindow), NULL);
			}
		}
		break;
	}
}

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDM_DOCKINSPECTOR:
		DockHostWindow_InvokeDockInspectorDialog(pDockHostWindow);
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
		DockHostWindow_OnMouseMove(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_MOUSELEAVE:
		DockHostWindow_SetCaptionHotButton(pDockHostWindow, NULL, DCB_NONE);
		return 0;
		break;

	case WM_LBUTTONDOWN:
		DockHostWindow_OnLButtonDown(pDockHostWindow, FALSE, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_LBUTTONUP:
		DockHostWindow_OnLButtonUp(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_CAPTURECHANGED:
			if (pDockHostWindow->fSplitDrag)
			{
				pDockHostWindow->fSplitDrag = FALSE;
				pDockHostWindow->pSplitNode = NULL;
				pDockHostWindow->iSplitDragStartGrip = 0;
			}
			DockHostWindow_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
			return 0;
			break;

	case WM_CONTEXTMENU:
			DockHostWindow_OnContextMenu(pDockHostWindow, (HWND)wParam, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam));
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
	DockHostWindow_ClearCaptionButtonState(pDockHostWindow);
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
	DockTargetHit targetHit = { 0 };
	if (!DockHostWindow_HitTestDockTarget(pDockHostWindow, ptScreen, &targetHit))
	{
		return DKS_NONE;
	}

	return targetHit.nDockSide;
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

static void DockHostWindow_GetGlobalTargetGuideRects(const RECT* pHostClient, RECT* pRectTop, RECT* pRectLeft, RECT* pRectRight, RECT* pRectBottom)
{
	if (!pHostClient)
	{
		return;
	}

	const int guideSize = DOCK_TARGET_GUIDE_SIZE;
	const int edgeInset = DOCK_TARGET_GUIDE_EDGE_MARGIN;
	const int cx = (pHostClient->left + pHostClient->right) / 2;
	const int cy = (pHostClient->top + pHostClient->bottom) / 2;

	if (pRectTop)
	{
		SetRect(pRectTop,
			cx - guideSize / 2,
			pHostClient->top + edgeInset,
			cx + guideSize / 2,
			pHostClient->top + edgeInset + guideSize);
	}
	if (pRectLeft)
	{
		SetRect(pRectLeft,
			pHostClient->left + edgeInset,
			cy - guideSize / 2,
			pHostClient->left + edgeInset + guideSize,
			cy + guideSize / 2);
	}
	if (pRectRight)
	{
		SetRect(pRectRight,
			pHostClient->right - edgeInset - guideSize,
			cy - guideSize / 2,
			pHostClient->right - edgeInset,
			cy + guideSize / 2);
	}
	if (pRectBottom)
	{
		SetRect(pRectBottom,
			cx - guideSize / 2,
			pHostClient->bottom - edgeInset - guideSize,
			cx + guideSize / 2,
			pHostClient->bottom - edgeInset);
	}
}

static void DockHostWindow_GetLocalTargetGuideRects(const RECT* pHostClient, const RECT* pAnchorRect, RECT* pRectCenter, RECT* pRectTop, RECT* pRectLeft, RECT* pRectRight, RECT* pRectBottom)
{
	if (!pHostClient || !pAnchorRect)
	{
		return;
	}

	const int guideSize = DOCK_TARGET_GUIDE_SIZE;
	const int guideStep = guideSize + DOCK_TARGET_GUIDE_GAP;
	int cx = (pAnchorRect->left + pAnchorRect->right) / 2;
	int cy = (pAnchorRect->top + pAnchorRect->bottom) / 2;

	cx = max(guideStep, min(cx, pHostClient->right - guideStep));
	cy = max(guideStep, min(cy, pHostClient->bottom - guideStep));

	if (pRectCenter)
	{
		SetRect(pRectCenter,
			cx - guideSize / 2,
			cy - guideSize / 2,
			cx + guideSize / 2,
			cy + guideSize / 2);
	}
	if (pRectTop)
	{
		SetRect(pRectTop,
			cx - guideSize / 2,
			cy - guideStep - guideSize / 2,
			cx + guideSize / 2,
			cy - guideStep + guideSize / 2);
	}
	if (pRectLeft)
	{
		SetRect(pRectLeft,
			cx - guideStep - guideSize / 2,
			cy - guideSize / 2,
			cx - guideStep + guideSize / 2,
			cy + guideSize / 2);
	}
	if (pRectRight)
	{
		SetRect(pRectRight,
			cx + guideStep - guideSize / 2,
			cy - guideSize / 2,
			cx + guideStep + guideSize / 2,
			cy + guideSize / 2);
	}
	if (pRectBottom)
	{
		SetRect(pRectBottom,
			cx - guideSize / 2,
			cy + guideStep - guideSize / 2,
			cx + guideSize / 2,
			cy + guideStep + guideSize / 2);
	}
}

static int DockHostWindow_HitTestGlobalTargetGuide(const RECT* pHostClient, POINT ptClient)
{
	if (!pHostClient || !PtInRect(pHostClient, ptClient))
	{
		return DKS_NONE;
	}

	RECT rcGuideTop = { 0 };
	RECT rcGuideLeft = { 0 };
	RECT rcGuideRight = { 0 };
	RECT rcGuideBottom = { 0 };
	DockHostWindow_GetGlobalTargetGuideRects(pHostClient, &rcGuideTop, &rcGuideLeft, &rcGuideRight, &rcGuideBottom);

	if (PtInRect(&rcGuideLeft, ptClient))
	{
		return DKS_LEFT;
	}
	if (PtInRect(&rcGuideRight, ptClient))
	{
		return DKS_RIGHT;
	}
	if (PtInRect(&rcGuideTop, ptClient))
	{
		return DKS_TOP;
	}
	if (PtInRect(&rcGuideBottom, ptClient))
	{
		return DKS_BOTTOM;
	}

	return DKS_NONE;
}

static int DockHostWindow_HitTestLocalTargetGuide(const RECT* pHostClient, const RECT* pAnchorRect, POINT ptClient)
{
	if (!pHostClient || !pAnchorRect)
	{
		return DKS_NONE;
	}

	RECT rcGuideCenter = { 0 };
	RECT rcGuideTop = { 0 };
	RECT rcGuideLeft = { 0 };
	RECT rcGuideRight = { 0 };
	RECT rcGuideBottom = { 0 };
	DockHostWindow_GetLocalTargetGuideRects(pHostClient, pAnchorRect, &rcGuideCenter, &rcGuideTop, &rcGuideLeft, &rcGuideRight, &rcGuideBottom);

	if (PtInRect(&rcGuideCenter, ptClient))
	{
		return DKS_CENTER;
	}

	if (PtInRect(&rcGuideLeft, ptClient))
	{
		return DKS_LEFT;
	}
	if (PtInRect(&rcGuideRight, ptClient))
	{
		return DKS_RIGHT;
	}
	if (PtInRect(&rcGuideTop, ptClient))
	{
		return DKS_TOP;
	}
	if (PtInRect(&rcGuideBottom, ptClient))
	{
		return DKS_BOTTOM;
	}

	return DKS_NONE;
}

static TreeNode* DockHostWindow_FindDockAnchorAtPoint(DockHostWindow* pDockHostWindow, POINT ptClient, RECT* pRectAnchor)
{
	if (pRectAnchor)
	{
		SetRectEmpty(pRectAnchor);
	}

	TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
	if (!pDockHostWindow || !pRoot)
	{
		return NULL;
	}

	TreeNode* pBestNode = NULL;
	RECT rcBest = { 0 };
	LONG bestArea = LONG_MAX;

	TreeTraversalRLOT traversal = { 0 };
	TreeTraversalRLOT_Init(&traversal, pRoot);

	TreeNode* pCurrent = NULL;
	while (pCurrent = TreeTraversalRLOT_GetNext(&traversal))
	{
		DockData* pDockData = pCurrent ? (DockData*)pCurrent->data : NULL;
		if (!pDockData || !pDockData->hWnd || !IsWindow(pDockData->hWnd))
		{
			continue;
		}

		if (DockHostWindow_IsWorkspaceWindow(pDockData->hWnd))
		{
			continue;
		}

		if (!DockNode_HasVisibleWindow(pCurrent))
		{
			continue;
		}

		RECT rc = pDockData->rc;
		if (!PtInRect(&rc, ptClient))
		{
			continue;
		}

		LONG area = Win32_Rect_GetWidth(&rc) * Win32_Rect_GetHeight(&rc);
		if (!pBestNode || area < bestArea)
		{
			pBestNode = pCurrent;
			rcBest = rc;
			bestArea = area;
		}
	}

	TreeTraversalRLOT_Destroy(&traversal);

	if (pRectAnchor && pBestNode)
	{
		*pRectAnchor = rcBest;
	}
	return pBestNode;
}

BOOL DockHostWindow_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit)
{
	if (!pDockHostWindow || !pTargetHit)
	{
		return FALSE;
	}

	memset(pTargetHit, 0, sizeof(*pTargetHit));

	HWND hWndDockHost = Window_GetHWND((Window*)pDockHostWindow);
	if (!hWndDockHost || !IsWindow(hWndDockHost))
	{
		return FALSE;
	}

	RECT rcHostScreen = { 0 };
	GetWindowRect(hWndDockHost, &rcHostScreen);
	if (!PtInRect(&rcHostScreen, ptScreen))
	{
		return FALSE;
	}

	POINT ptClient = ptScreen;
	ScreenToClient(hWndDockHost, &ptClient);

	RECT rcClient = { 0 };
	GetClientRect(hWndDockHost, &rcClient);

	RECT rcAnchor = { 0 };
	TreeNode* pAnchorNode = DockHostWindow_FindDockAnchorAtPoint(pDockHostWindow, ptClient, &rcAnchor);
	if (pAnchorNode && pAnchorNode->data)
	{
		DockData* pAnchorData = (DockData*)pAnchorNode->data;
		pTargetHit->bLocalTarget = TRUE;
		pTargetHit->hWndAnchor = pAnchorData->hWnd;
		pTargetHit->rcAnchorClient = rcAnchor;

			int localSide = DockHostWindow_HitTestLocalTargetGuide(&rcClient, &rcAnchor, ptClient);
			if (localSide != DKS_NONE)
			{
				pTargetHit->nDockSide = localSide;
				if (localSide == DKS_CENTER)
				{
					pTargetHit->rcPreviewClient = rcAnchor;
				}
				else if (!DockLayout_GetDockPreviewRect(&rcAnchor, localSide, &pTargetHit->rcPreviewClient))
				{
					pTargetHit->rcPreviewClient = rcAnchor;
				}
				return TRUE;
			}
	}

	int globalSide = DockHostWindow_HitTestGlobalTargetGuide(&rcClient, ptClient);
	if (globalSide != DKS_NONE)
	{
		pTargetHit->nDockSide = globalSide;
		pTargetHit->bLocalTarget = FALSE;
		SetRectEmpty(&pTargetHit->rcAnchorClient);
		SetRectEmpty(&pTargetHit->rcPreviewClient);
		DockLayout_GetDockPreviewRect(&rcClient, globalSide, &pTargetHit->rcPreviewClient);
		return TRUE;
	}

	/*
	 * Keep guides visible anywhere inside the host, but do not activate a drop
	 * preview until the cursor is over a concrete guide icon.
	 */
	return TRUE;
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

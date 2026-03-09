#include "precomp.h"

#include "dockhostautohide.h"

#include "dockhosttree.h"
#include "dockhostzone.h"
#include "docklayout.h"
#include "dockpolicy.h"
#include "theme.h"
#include "toolwndframe.h"
#include "panitentapp.h"
#include "win32/util.h"

#define DOCKHOST_AUTOHIDE_MIN_SIZE 96
#define DOCKHOST_AUTOHIDE_CAPTION_HEIGHT 14
#define DOCKHOST_AUTOHIDE_BORDER_INSET 3
#define DOCKHOST_AUTOHIDE_BUTTON_SIZE 14
#define DOCKHOST_AUTOHIDE_BUTTON_SPACING 3

void DockHostWindow_SetAutoHideHotButton(DockHostWindow* pDockHostWindow, int nButton)
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

void DockHostWindow_SetAutoHidePressedButton(DockHostWindow* pDockHostWindow, int nButton)
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

void DockHostWindow_ClearAutoHideCaptionState(DockHostWindow* pDockHostWindow)
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

BOOL DockHostWindow_GetAutoHideOverlayRect(DockHostWindow* pDockHostWindow, int nDockSide, RECT* pRect)
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

	iOverlaySize = DockLayout_ClampSplitGrip(iSpan, iOverlaySize, DOCKHOST_AUTOHIDE_MIN_SIZE);
	iOverlaySize = max(DOCKHOST_AUTOHIDE_MIN_SIZE, min(iOverlaySize, iSpan));

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

BOOL DockHostWindow_BuildAutoHideOverlayLayout(DockHostWindow* pDockHostWindow, CaptionFrameLayout* pLayout)
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
	if (bShowClose && nButtons < ARRAYSIZE(buttons))
	{
		buttons[nButtons++] = (CaptionButton){ (SIZE){ DOCKHOST_AUTOHIDE_BUTTON_SIZE, DOCKHOST_AUTOHIDE_BUTTON_SIZE }, CAPTION_GLYPH_CLOSE_TILE, DCB_CLOSE };
	}
	if (bShowPin && nButtons < ARRAYSIZE(buttons))
	{
		buttons[nButtons++] = (CaptionButton){ (SIZE){ DOCKHOST_AUTOHIDE_BUTTON_SIZE, DOCKHOST_AUTOHIDE_BUTTON_SIZE }, CAPTION_GLYPH_PIN_DIAGONAL_TILE, DCB_PIN };
	}
	if (nButtons < ARRAYSIZE(buttons))
	{
		buttons[nButtons++] = (CaptionButton){ (SIZE){ DOCKHOST_AUTOHIDE_BUTTON_SIZE, DOCKHOST_AUTOHIDE_BUTTON_SIZE }, CAPTION_GLYPH_CHEVRON_TILE, DCB_MORE };
	}

	CaptionFrameMetrics metrics = { 0 };
	metrics.borderSize = DOCKHOST_AUTOHIDE_BORDER_INSET;
	metrics.captionHeight = DOCKHOST_AUTOHIDE_CAPTION_HEIGHT;
	metrics.buttonSpacing = DOCKHOST_AUTOHIDE_BUTTON_SPACING;
	metrics.textPaddingLeft = DOCKHOST_AUTOHIDE_BORDER_INSET;
	metrics.textPaddingRight = DOCKHOST_AUTOHIDE_BORDER_INSET;
	metrics.textPaddingY = 0;

	RECT rcClient = { 0 };
	GetClientRect(hWndOverlayHost, &rcClient);
	return CaptionFrame_BuildLayout(&rcClient, &metrics, buttons, nButtons, pLayout);
}

void DockHostWindow_HideAutoHideOverlay(DockHostWindow* pDockHostWindow)
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

void DockHostWindow_UpdateAutoHideOverlay(DockHostWindow* pDockHostWindow)
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
	rcChild.top += DOCKHOST_AUTOHIDE_CAPTION_HEIGHT + 1;
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

BOOL DockHostWindow_ShowAutoHideOverlay(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab)
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

#include "precomp.h"

#include "dockhostautohide.h"

#include "dockhostpanelmenu.h"
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
static const WCHAR szAutoHideOverlayHostClassName[] = L"__DockAutoHideOverlayHost";

static void DockAutoHideOverlayHost_OnPaint(DockHostWindow* pDockHostWindow, HWND hWnd);
static LRESULT CALLBACK DockAutoHideOverlayHostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL DockHostWindow_EnsureAutoHideOverlayHost(DockHostWindow* pDockHostWindow)
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
		DockHostWindow_SetAutoHideHotButton(pDockHostWindow, CaptionFrame_HitTestButton(&layout, pt));
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
				DockHostPanelMenu_TogglePanelPinned(pDockHostWindow, pPanelNode);
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
			DockHostPanelMenu_Show(pDockHostWindow, pPanelNode, ptScreen);
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

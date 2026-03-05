#include "precomp.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "win32/util.h"
#include "floatingwindowcontainer.h"
#include "dockhost.h"
#include "docklayout.h"
#include "resource.h"
#include "toolwndframe.h"
#include "workspacecontainer.h"
#include "panitentapp.h"
#include "util/assert.h"

#define HTMORE 23

#define GLYPH_CHEVRON_TILE 0
#define GLYPH_MINIMIZE_TILE 2
#define GLYPH_MAXIMIZE_TILE 3
#define GLYPH_CLOSE_TILE 4
#define DOCK_TARGET_GUIDE_SIZE 30
#define DOCK_TARGET_GUIDE_GAP 10

static const WCHAR szClassName[] = L"__FloatingWindowContainer";

/* Private forward declarations */
FloatingWindowContainer* FloatingWindowContainer_Create();
void FloatingWindowContainer_Init(FloatingWindowContainer*);
static BOOL FloatingWindowContainer_AttemptDock(FloatingWindowContainer* pFloatingWindowContainer);
static void FloatingWindowContainer_UpdateDockPreviewOverlay(FloatingWindowContainer* pFloatingWindowContainer);
static void FloatingWindowContainer_DestroyDockPreviewOverlay(void);
static WorkspaceContainer* FloatingWindowContainer_GetWorkspaceChild(FloatingWindowContainer* pFloatingWindowContainer);
static BOOL FloatingWindowContainer_AttemptDockDocument(FloatingWindowContainer* pFloatingWindowContainer, BOOL bForceMainWorkspace);
static void FloatingWindowContainer_UpdateDocumentDockPreviewOverlay(FloatingWindowContainer* pFloatingWindowContainer);

void FloatingWindowContainer_PreRegister(LPWNDCLASSEX);
void FloatingWindowContainer_PreCreate(LPCREATESTRUCT);

BOOL FloatingWindowContainer_OnCreate(FloatingWindowContainer*, LPCREATESTRUCT);
void FloatingWindowContainer_OnPaint(FloatingWindowContainer*);
void FloatingWindowContainer_OnLButtonUp(FloatingWindowContainer*, int, int);
void FloatingWindowContainer_OnRButtonUp(FloatingWindowContainer*, int, int);
void FloatingWindowContainer_OnContextMenu(FloatingWindowContainer*, int, int);
void FloatingWindowContainer_OnDestroy(FloatingWindowContainer*);
void FloatingWindowContainer_OnSize(FloatingWindowContainer* pFloatingWindowContainer, UINT state, int cx, int cy);
LRESULT CALLBACK FloatingWindowContainer_UserProc(FloatingWindowContainer*, HWND hWnd, UINT, WPARAM, LPARAM);

BOOL FloatingWindowContainer_OnNCCreate(FloatingWindowContainer* pFloatingWindowContainer, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(lpcs);

    HWND hWnd = pFloatingWindowContainer->base.hWnd;

    SetWindowTheme(hWnd, L"", L"");

    return TRUE;
}

int g_borderSize = 3;
int g_borderGripSize = 8;
int g_captionHeight = 14;

static int FloatingWindowContainer_GetCaptionHeight(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (!pFloatingWindowContainer)
    {
        return g_captionHeight;
    }

    if (pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        return 22;
    }

    return g_captionHeight;
}

static const WCHAR szDockPreviewOverlayClassName[] = L"__DockPreviewOverlay";
static HWND g_hWndDockPreviewOverlay = NULL;

static LRESULT CALLBACK DockPreviewOverlay_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hWnd);
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static BOOL DockPreviewOverlay_EnsureClass(void)
{
	WNDCLASSEX wcex = { 0 };
	if (GetClassInfoEx(GetModuleHandle(NULL), szDockPreviewOverlayClassName, &wcex))
	{
		return TRUE;
	}

	wcex.cbSize = sizeof(wcex);
	wcex.lpfnWndProc = DockPreviewOverlay_WndProc;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszClassName = szDockPreviewOverlayClassName;
	return RegisterClassEx(&wcex) != 0;
}

static HWND DockPreviewOverlay_EnsureWindow(void)
{
	if (g_hWndDockPreviewOverlay && IsWindow(g_hWndDockPreviewOverlay))
	{
		return g_hWndDockPreviewOverlay;
	}

	if (!DockPreviewOverlay_EnsureClass())
	{
		return NULL;
	}

	g_hWndDockPreviewOverlay = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		szDockPreviewOverlayClassName,
		L"DockPreviewOverlay",
		WS_POPUP,
		0, 0, 0, 0,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL);

	return g_hWndDockPreviewOverlay;
}

static void DockPreviewOverlay_FillRectARGB(DWORD* pBits, int width, int height, const RECT* pRect, BYTE a, BYTE r, BYTE g, BYTE b)
{
	if (!pBits || !pRect || width <= 0 || height <= 0)
	{
		return;
	}

	RECT rc = *pRect;
	rc.left = max(0, min(rc.left, width));
	rc.right = max(0, min(rc.right, width));
	rc.top = max(0, min(rc.top, height));
	rc.bottom = max(0, min(rc.bottom, height));
	if (rc.left >= rc.right || rc.top >= rc.bottom)
	{
		return;
	}

	DWORD color = ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
	for (int y = rc.top; y < rc.bottom; ++y)
	{
		DWORD* pRow = pBits + y * width;
		for (int x = rc.left; x < rc.right; ++x)
		{
			pRow[x] = color;
		}
	}
}

static void FloatingWindowContainer_DestroyDockPreviewOverlay(void)
{
	if (g_hWndDockPreviewOverlay && IsWindow(g_hWndDockPreviewOverlay))
	{
		DestroyWindow(g_hWndDockPreviewOverlay);
	}

	g_hWndDockPreviewOverlay = NULL;
}

static WorkspaceContainer* FloatingWindowContainer_GetWorkspaceChild(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (!pFloatingWindowContainer || !pFloatingWindowContainer->hWndChild || !IsWindow(pFloatingWindowContainer->hWndChild))
    {
        return NULL;
    }

    Window* pWindow = WindowMap_Get(pFloatingWindowContainer->hWndChild);
    if (!pWindow)
    {
        return NULL;
    }

    return (WorkspaceContainer*)pWindow;
}

static void FloatingWindowContainer_UpdateDocumentDockPreviewOverlay(FloatingWindowContainer* pFloatingWindowContainer)
{
    WorkspaceContainer* pWorkspaceSource = FloatingWindowContainer_GetWorkspaceChild(pFloatingWindowContainer);
    if (!pWorkspaceSource)
    {
        FloatingWindowContainer_DestroyDockPreviewOverlay();
        return;
    }

    POINT ptCursor = { 0 };
    if (!GetCursorPos(&ptCursor))
    {
        FloatingWindowContainer_DestroyDockPreviewOverlay();
        return;
    }

    WorkspaceContainer* pWorkspaceTarget = WorkspaceContainer_FindDropTargetAtScreenPoint(pWorkspaceSource, ptCursor);
    if (!pWorkspaceTarget || pWorkspaceTarget == pWorkspaceSource)
    {
        FloatingWindowContainer_DestroyDockPreviewOverlay();
        return;
    }

    HWND hWndTargetWorkspace = Window_GetHWND((Window*)pWorkspaceTarget);
    if (!hWndTargetWorkspace || !IsWindow(hWndTargetWorkspace))
    {
        FloatingWindowContainer_DestroyDockPreviewOverlay();
        return;
    }

    RECT rcTargetScreen = { 0 };
    GetWindowRect(hWndTargetWorkspace, &rcTargetScreen);
    int width = Win32_Rect_GetWidth(&rcTargetScreen);
    int height = Win32_Rect_GetHeight(&rcTargetScreen);
    if (width <= 0 || height <= 0)
    {
        FloatingWindowContainer_DestroyDockPreviewOverlay();
        return;
    }

    HWND hWndOverlay = DockPreviewOverlay_EnsureWindow();
    if (!hWndOverlay)
    {
        return;
    }

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    DWORD* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
    memset(pBits, 0, (size_t)width * (size_t)height * sizeof(DWORD));

    RECT rcPreview = { 0, 0, width, height };
    DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcPreview, 95, 0x4d, 0x8e, 0xd6);

    RECT rcBorderTop = { rcPreview.left, rcPreview.top, rcPreview.right, rcPreview.top + 2 };
    RECT rcBorderBottom = { rcPreview.left, rcPreview.bottom - 2, rcPreview.right, rcPreview.bottom };
    RECT rcBorderLeft = { rcPreview.left, rcPreview.top, rcPreview.left + 2, rcPreview.bottom };
    RECT rcBorderRight = { rcPreview.right - 2, rcPreview.top, rcPreview.right, rcPreview.bottom };
    DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderTop, 190, 0x73, 0xb3, 0xf2);
    DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderBottom, 190, 0x73, 0xb3, 0xf2);
    DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderLeft, 190, 0x73, 0xb3, 0xf2);
    DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderRight, 190, 0x73, 0xb3, 0xf2);

    int guideSize = 28;
    RECT rcGuideCenter = {
        width / 2 - guideSize / 2,
        height / 2 - guideSize / 2,
        width / 2 + guideSize / 2,
        height / 2 + guideSize / 2
    };
    DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideCenter, 210, 0x73, 0xb3, 0xf2);

    POINT ptPos = { rcTargetScreen.left, rcTargetScreen.top };
    SIZE sizeWnd = { width, height };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blendFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hWndOverlay, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blendFunction, ULW_ALPHA);
    ShowWindow(hWndOverlay, SW_SHOWNA);

    SelectObject(hdcMem, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

static void FloatingWindowContainer_UpdateDockPreviewOverlay(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (pFloatingWindowContainer && pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        FloatingWindowContainer_UpdateDocumentDockPreviewOverlay(pFloatingWindowContainer);
        return;
    }

	if (!pFloatingWindowContainer || !pFloatingWindowContainer->pDockHostTarget)
	{
		FloatingWindowContainer_DestroyDockPreviewOverlay();
		return;
	}

	HWND hWndDockHost = Window_GetHWND((Window*)pFloatingWindowContainer->pDockHostTarget);
	if (!hWndDockHost || !IsWindow(hWndDockHost))
	{
		FloatingWindowContainer_DestroyDockPreviewOverlay();
		return;
	}

	POINT ptCursor = { 0 };
	if (!GetCursorPos(&ptCursor))
	{
		FloatingWindowContainer_DestroyDockPreviewOverlay();
		return;
	}

	DockTargetHit dockTarget = { 0 };
	if (!DockHostWindow_HitTestDockTarget(pFloatingWindowContainer->pDockHostTarget, ptCursor, &dockTarget) ||
		dockTarget.nDockSide == DKS_NONE)
	{
		FloatingWindowContainer_DestroyDockPreviewOverlay();
		return;
	}

	RECT rcHostScreen = { 0 };
	GetWindowRect(hWndDockHost, &rcHostScreen);
	int width = Win32_Rect_GetWidth(&rcHostScreen);
	int height = Win32_Rect_GetHeight(&rcHostScreen);
	if (width <= 0 || height <= 0)
	{
		FloatingWindowContainer_DestroyDockPreviewOverlay();
		return;
	}

	HWND hWndOverlay = DockPreviewOverlay_EnsureWindow();
	if (!hWndOverlay)
	{
		return;
	}

	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	DWORD* pBits = NULL;
	HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
	memset(pBits, 0, (size_t)width * (size_t)height * sizeof(DWORD));

	RECT rcHostLocal = { 0, 0, width, height };
	RECT rcPreview = { 0 };
	rcPreview = dockTarget.rcPreviewClient;
	if (IsRectEmpty(&rcPreview))
	{
		DockLayout_GetDockPreviewRect(&rcHostLocal, dockTarget.nDockSide, &rcPreview);
	}
	if (!IsRectEmpty(&rcPreview))
	{
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcPreview, 110, 0x4d, 0x8e, 0xd6);

		RECT rcBorderTop = { rcPreview.left, rcPreview.top, rcPreview.right, rcPreview.top + 2 };
		RECT rcBorderBottom = { rcPreview.left, rcPreview.bottom - 2, rcPreview.right, rcPreview.bottom };
		RECT rcBorderLeft = { rcPreview.left, rcPreview.top, rcPreview.left + 2, rcPreview.bottom };
		RECT rcBorderRight = { rcPreview.right - 2, rcPreview.top, rcPreview.right, rcPreview.bottom };
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderTop, 190, 0x73, 0xb3, 0xf2);
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderBottom, 190, 0x73, 0xb3, 0xf2);
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderLeft, 190, 0x73, 0xb3, 0xf2);
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcBorderRight, 190, 0x73, 0xb3, 0xf2);
	}

	const int guideSize = DOCK_TARGET_GUIDE_SIZE;
	const int guideGap = DOCK_TARGET_GUIDE_GAP;
	int cx = width / 2;
	int cy = height / 2;
	if (dockTarget.bLocalTarget && !IsRectEmpty(&dockTarget.rcAnchorClient))
	{
		cx = (dockTarget.rcAnchorClient.left + dockTarget.rcAnchorClient.right) / 2;
		cy = (dockTarget.rcAnchorClient.top + dockTarget.rcAnchorClient.bottom) / 2;
		cx = max(guideSize * 2, min(cx, width - guideSize * 2));
		cy = max(guideSize * 2, min(cy, height - guideSize * 2));

		RECT rcAnchorOutline = dockTarget.rcAnchorClient;
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcAnchorOutline, 40, 0x5d, 0x78, 0x98);
		RECT rcAnchorTop = { rcAnchorOutline.left, rcAnchorOutline.top, rcAnchorOutline.right, rcAnchorOutline.top + 1 };
		RECT rcAnchorBottom = { rcAnchorOutline.left, rcAnchorOutline.bottom - 1, rcAnchorOutline.right, rcAnchorOutline.bottom };
		RECT rcAnchorLeft = { rcAnchorOutline.left, rcAnchorOutline.top, rcAnchorOutline.left + 1, rcAnchorOutline.bottom };
		RECT rcAnchorRight = { rcAnchorOutline.right - 1, rcAnchorOutline.top, rcAnchorOutline.right, rcAnchorOutline.bottom };
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcAnchorTop, 170, 0x78, 0xb8, 0xf8);
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcAnchorBottom, 170, 0x78, 0xb8, 0xf8);
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcAnchorLeft, 170, 0x78, 0xb8, 0xf8);
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcAnchorRight, 170, 0x78, 0xb8, 0xf8);
	}
	RECT rcGuideLeft = { cx - guideGap - guideSize * 2, cy - guideSize / 2, cx - guideGap - guideSize, cy + guideSize / 2 };
	RECT rcGuideRight = { cx + guideGap + guideSize, cy - guideSize / 2, cx + guideGap + guideSize * 2, cy + guideSize / 2 };
	RECT rcGuideTop = { cx - guideSize / 2, cy - guideGap - guideSize * 2, cx + guideSize / 2, cy - guideGap - guideSize };
	RECT rcGuideBottom = { cx - guideSize / 2, cy + guideGap + guideSize, cx + guideSize / 2, cy + guideGap + guideSize * 2 };
	RECT rcGuideCenter = { cx - guideSize / 2, cy - guideSize / 2, cx + guideSize / 2, cy + guideSize / 2 };

	if (!dockTarget.bLocalTarget)
	{
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideCenter, 85, 0x55, 0x6b, 0x88);
	}
	DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideLeft, 75, 0x4f, 0x62, 0x7a);
	DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideRight, 75, 0x4f, 0x62, 0x7a);
	DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideTop, 75, 0x4f, 0x62, 0x7a);
	DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideBottom, 75, 0x4f, 0x62, 0x7a);

	switch (dockTarget.nDockSide)
	{
	case DKS_LEFT:
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideLeft, 190, 0x73, 0xb3, 0xf2);
		break;
	case DKS_RIGHT:
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideRight, 190, 0x73, 0xb3, 0xf2);
		break;
	case DKS_TOP:
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideTop, 190, 0x73, 0xb3, 0xf2);
		break;
	case DKS_BOTTOM:
		DockPreviewOverlay_FillRectARGB(pBits, width, height, &rcGuideBottom, 190, 0x73, 0xb3, 0xf2);
		break;
	}

	POINT ptPos = { rcHostScreen.left, rcHostScreen.top };
	SIZE sizeWnd = { width, height };
	POINT ptSrc = { 0, 0 };
	BLENDFUNCTION blendFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	UpdateLayeredWindow(hWndOverlay, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blendFunction, ULW_ALPHA);
	ShowWindow(hWndOverlay, SW_SHOWNA);

	SelectObject(hdcMem, hOld);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
}

LRESULT FloatingWindowContainer_OnNCCalcSize(FloatingWindowContainer* pFloatingWindowContainer, BOOL bProcess, LPARAM lParam)
{
    HWND hWnd = pFloatingWindowContainer->base.hWnd;
    DWORD dwStyle = GetWindowStyle(hWnd);
    int captionHeight = FloatingWindowContainer_GetCaptionHeight(pFloatingWindowContainer);

    if (bProcess)
    {
        NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;

        if (dwStyle & WS_THICKFRAME)
        {
            params->rgrc[0].left += g_borderSize;
            params->rgrc[0].top += g_borderSize;
            params->rgrc[0].right -= g_borderSize;
            params->rgrc[0].bottom -= g_borderSize;
        }
        else {
            params->rgrc[0].left += 1;
            params->rgrc[0].top += 1;
            params->rgrc[0].right -= 1;
            params->rgrc[0].bottom -= 1;
        }

        if (dwStyle & WS_CAPTION)
        {
            params->rgrc[0].top += captionHeight + g_borderSize;
            if (!(dwStyle & WS_THICKFRAME))
            {
                params->rgrc[0].top += g_borderSize;
            }
        }        

        return WVR_ALIGNTOP;
    }

    RECT* rc = (RECT*)lParam;

    rc->left += g_borderSize;
    rc->top += g_borderSize;
    
    if (dwStyle & WS_CAPTION)
    {
        rc->top += captionHeight + g_borderSize;
        if (!(dwStyle & WS_THICKFRAME))
        {
            rc->top += g_borderSize;
        }
    }
    
    rc->right -= g_borderSize;
    rc->bottom -= g_borderSize;

    return 0;
}

LRESULT CaptionRightContainerHitTest()
{
    return HTNOWHERE;
}

static int FloatingWindowContainer_BuildCaptionButtons(FloatingWindowContainer* pFloatingWindowContainer, CaptionButton* pButtons, int cButtons)
{
    if (!pButtons || cButtons < 3)
    {
        return 0;
    }

    pButtons[0] = (CaptionButton){ (SIZE){ 14, 14 }, GLYPH_CLOSE_TILE, HTCLOSE };
    pButtons[1] = (CaptionButton){ (SIZE){ 14, 14 }, GLYPH_MAXIMIZE_TILE, HTMAXBUTTON };
    if (pFloatingWindowContainer && pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        pButtons[2] = (CaptionButton){ (SIZE){ 14, 14 }, GLYPH_MINIMIZE_TILE, HTMINBUTTON };
    }
    else {
        pButtons[2] = (CaptionButton){ (SIZE){ 14, 14 }, GLYPH_CHEVRON_TILE, HTMORE };
    }

    return 3;
}

static BOOL FloatingWindowContainer_BuildCaptionLayout(FloatingWindowContainer* pFloatingWindowContainer, CaptionFrameLayout* pLayout)
{
    if (!pFloatingWindowContainer || !pLayout)
    {
        return FALSE;
    }

    HWND hWnd = pFloatingWindowContainer->base.hWnd;
    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    rcWindow.right -= rcWindow.left;
    rcWindow.bottom -= rcWindow.top;
    rcWindow.left = 0;
    rcWindow.top = 0;

    CaptionFrameMetrics metrics = { 0 };
    metrics.borderSize = g_borderSize;
    metrics.captionHeight = FloatingWindowContainer_GetCaptionHeight(pFloatingWindowContainer);
    metrics.buttonSpacing = 3;
    metrics.textPaddingX = g_borderSize;
    metrics.textPaddingY = 0;

    CaptionButton buttons[3] = { 0 };
    int nButtons = FloatingWindowContainer_BuildCaptionButtons(pFloatingWindowContainer, buttons, ARRAYSIZE(buttons));
    return CaptionFrame_BuildLayout(&rcWindow, &metrics, buttons, nButtons, pLayout);
}

void FloatingWindowContainer_OnNCPaint(FloatingWindowContainer* pFloatingWindowContainer, HRGN hUpdRegion)
{
    UNREFERENCED_PARAMETER(hUpdRegion);

    HWND hWnd = pFloatingWindowContainer->base.hWnd;
    HDC hdc = GetWindowDC(hWnd);

    DWORD dwStyle = GetWindowStyle(hWnd);

    CaptionFramePalette palette = { 0 };
    palette.frameFill = Win32_HexToCOLORREF(L"#9185be");
    palette.border = Win32_HexToCOLORREF(L"#6d648e");
    palette.captionFill = Win32_HexToCOLORREF(L"#9185be");
    palette.text = COLORREF_WHITE;

    if (dwStyle & WS_CAPTION)
    {
        CaptionFrameLayout layout = { 0 };
        if (!FloatingWindowContainer_BuildCaptionLayout(pFloatingWindowContainer, &layout))
        {
            ReleaseDC(hWnd, hdc);
            return;
        }

        WCHAR szTitle[MAX_PATH] = L"";
        if (pFloatingWindowContainer->hWndChild && IsWindow(pFloatingWindowContainer->hWndChild))
        {
            GetWindowText(pFloatingWindowContainer->hWndChild, szTitle, ARRAYSIZE(szTitle));
        }

        CaptionFrame_Draw(hdc, &layout, &palette, szTitle, PanitentApp_GetUIFont(PanitentApp_Instance()));
    }
    else {
        RECT rc = { 0 };
        GetWindowRect(hWnd, &rc);
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        rc.top = rc.left = 0;
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SelectObject(hdc, GetStockObject(DC_PEN));
        SetDCBrushColor(hdc, palette.frameFill);
        SetDCPenColor(hdc, palette.border);
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }

    ReleaseDC(hWnd, hdc);
}

LRESULT FloatingWindowContainer_OnNCHitTest(FloatingWindowContainer* pFloatingWindowContainer, int x, int y)
{
    HWND hWnd = pFloatingWindowContainer->base.hWnd;

    DWORD dwStyle = GetWindowStyle(hWnd);

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);

    if (dwStyle & WS_CAPTION)
    {
        CaptionFrameLayout layout = { 0 };
        if (FloatingWindowContainer_BuildCaptionLayout(pFloatingWindowContainer, &layout))
        {
            POINT ptLocal = { x - windowRect.left, y - windowRect.top };
            int nButtonHit = CaptionFrame_HitTestButton(&layout, ptLocal);
            if (nButtonHit != HTNOWHERE)
            {
                return nButtonHit;
            }

            if (PtInRect(&layout.rcCaption, ptLocal))
            {
                return HTCAPTION;
            }
        }
    }

    if (dwStyle & WS_THICKFRAME)
    {
        if (!(x >= windowRect.left + g_borderSize && x < windowRect.right - g_borderSize && y >= windowRect.top + g_borderSize && y < windowRect.bottom - g_borderSize))
        {
            // Check if the cursor is on the top-left corner
            if (x >= windowRect.left && x <= windowRect.left + g_borderGripSize &&
                y >= windowRect.top && y <= windowRect.top + g_borderGripSize) {
                return HTTOPLEFT;
            }
            // Check if the cursor is on the top-right corner
            else if (x >= windowRect.right - g_borderGripSize && x <= windowRect.right &&
                y >= windowRect.top && y <= windowRect.top + g_borderGripSize) {
                return HTTOPRIGHT;
            }
            // Check if the cursor is on the bottom-left corner
            else if (x >= windowRect.left && x <= windowRect.left + g_borderGripSize &&
                y >= windowRect.bottom - g_borderGripSize && y <= windowRect.bottom) {
                return HTBOTTOMLEFT;
            }
            // Check if the cursor is on the bottom-right corner
            else if (x >= windowRect.right - g_borderGripSize && x <= windowRect.right &&
                y >= windowRect.bottom - g_borderGripSize && y <= windowRect.bottom) {
                return HTBOTTOMRIGHT;
            }
            else if (x >= windowRect.left && x <= windowRect.left + g_borderGripSize) {
                return HTLEFT;
            }
            // Check if the cursor is on the right border
            else if (x >= windowRect.right - g_borderGripSize && x <= windowRect.right) {
                return HTRIGHT;
            }
            // Check if the cursor is on the top border
            else if (y >= windowRect.top && y <= windowRect.top + g_borderGripSize) {
                return HTTOP;
            }
            // Check if the cursor is on the bottom border
            else if (y >= windowRect.bottom - g_borderGripSize && y <= windowRect.bottom) {
                return HTBOTTOM;
            }
        }
    }
    
    return HTCLIENT;
}

FloatingWindowContainer* FloatingWindowContainer_Create()
{
    FloatingWindowContainer* pFloatingWindowContainer = (FloatingWindowContainer*)malloc(sizeof(FloatingWindowContainer));

    if (pFloatingWindowContainer)
    {
        memset(pFloatingWindowContainer, 0, sizeof(FloatingWindowContainer));

        FloatingWindowContainer_Init(pFloatingWindowContainer);
    }

    return pFloatingWindowContainer;
}

void FloatingWindowContainer_Init(FloatingWindowContainer* window)
{
    Window_Init(&window->base);

    window->base.szClassName = szClassName;

    window->base.OnCreate = (FnWindowOnCreate)FloatingWindowContainer_OnCreate;
    window->base.OnDestroy = (FnWindowOnDestroy)FloatingWindowContainer_OnDestroy;
    window->base.OnPaint = (FnWindowOnPaint)FloatingWindowContainer_OnPaint;
    window->base.OnSize = (FnWindowOnSize)FloatingWindowContainer_OnSize;
    window->base.PreRegister = (FnWindowPreRegister)FloatingWindowContainer_PreRegister;
    window->base.PreCreate = (FnWindowPreCreate)FloatingWindowContainer_PreCreate;
    window->base.UserProc = (FnWindowUserProc)FloatingWindowContainer_UserProc;

    window->bPinned = FALSE;
    window->pDockHostTarget = NULL;
    window->iDockSizeHint = 280;
    window->nDockPolicy = FLOAT_DOCK_POLICY_PANEL;
}

void FloatingWindowContainer_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE);
    lpwcex->lpszClassName = szClassName;
}

BOOL FloatingWindowContainer_OnCreate(FloatingWindowContainer* window, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lpcs);

    return TRUE;
}

void FloatingWindowContainer_OnPaint(FloatingWindowContainer* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void FloatingWindowContainer_OnLButtonUp(FloatingWindowContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void FloatingWindowContainer_OnRButtonUp(FloatingWindowContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void FloatingWindowContainer_OnContextMenu(FloatingWindowContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void FloatingWindowContainer_OnDestroy(FloatingWindowContainer* window)
{
    UNREFERENCED_PARAMETER(window);
    FloatingWindowContainer_DestroyDockPreviewOverlay();
}


#define IDM_FLOAT_DOCK 1001
#define IDM_FLOAT_AUTOHIDE 1002
#define IDM_FLOAT_MOVETONEW 1003
#define IDM_FLOAT_CLOSE 1004

static BOOL FloatingWindowContainer_DockByCenter(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (pFloatingWindowContainer && pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        return FloatingWindowContainer_AttemptDockDocument(pFloatingWindowContainer, TRUE);
    }

    if (!pFloatingWindowContainer ||
        !pFloatingWindowContainer->pDockHostTarget ||
        !pFloatingWindowContainer->hWndChild ||
        !IsWindow(pFloatingWindowContainer->hWndChild))
    {
        return FALSE;
    }

    RECT rcFloating = { 0 };
    GetWindowRect(Window_GetHWND((Window*)pFloatingWindowContainer), &rcFloating);
	POINT ptCenter = {
		rcFloating.left + Win32_Rect_GetWidth(&rcFloating) / 2,
		rcFloating.top + Win32_Rect_GetHeight(&rcFloating) / 2
	};

	DockTargetHit dockTarget = { 0 };
	if (!DockHostWindow_HitTestDockTarget(pFloatingWindowContainer->pDockHostTarget, ptCenter, &dockTarget))
	{
		dockTarget.nDockSide = DKS_LEFT;
	}
	int nDockSide = dockTarget.nDockSide;

	int iDockSize = pFloatingWindowContainer->iDockSizeHint;
	if (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT)
    {
        iDockSize = max(iDockSize, Win32_Rect_GetWidth(&rcFloating));
    }
    else {
        iDockSize = max(iDockSize, Win32_Rect_GetHeight(&rcFloating));
    }
    iDockSize = max(iDockSize, 180);

	HWND hWndChild = pFloatingWindowContainer->hWndChild;
	pFloatingWindowContainer->hWndChild = NULL;
	if (!DockHostWindow_DockHWNDToTarget(pFloatingWindowContainer->pDockHostTarget, hWndChild, &dockTarget, iDockSize))
	{
		pFloatingWindowContainer->hWndChild = hWndChild;
		return FALSE;
    }

    return TRUE;
}

static void FloatingWindowContainer_ShowPanelMenu(FloatingWindowContainer* pFloatingWindowContainer, POINT ptScreen)
{
    if (!pFloatingWindowContainer)
    {
        return;
    }

    HMENU hPopup = CreatePopupMenu();
    if (!hPopup)
    {
        return;
    }

    BOOL bCanDock = FALSE;
    if (pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        bCanDock = (pFloatingWindowContainer->hWndChild && IsWindow(pFloatingWindowContainer->hWndChild));
    }
    else {
        bCanDock = (pFloatingWindowContainer->pDockHostTarget &&
            pFloatingWindowContainer->hWndChild &&
            IsWindow(pFloatingWindowContainer->hWndChild));
    }

    AppendMenu(hPopup, MF_STRING | (bCanDock ? MF_ENABLED : MF_GRAYED), IDM_FLOAT_DOCK, L"Doc&k");
    AppendMenu(hPopup, MF_STRING | MF_GRAYED, IDM_FLOAT_AUTOHIDE, L"&Auto Hide");
    AppendMenu(hPopup, MF_STRING | MF_GRAYED, IDM_FLOAT_MOVETONEW, L"Move To &New Window");
    AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(hPopup, MF_STRING, IDM_FLOAT_CLOSE, L"&Close\tShift+Esc");

    UINT uCmd = (UINT)TrackPopupMenu(hPopup, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN,
        ptScreen.x, ptScreen.y, 0, pFloatingWindowContainer->base.hWnd, NULL);
    DestroyMenu(hPopup);

    switch (uCmd)
    {
    case IDM_FLOAT_DOCK:
        if (FloatingWindowContainer_DockByCenter(pFloatingWindowContainer))
        {
            DestroyWindow(pFloatingWindowContainer->base.hWnd);
        }
        break;

    case IDM_FLOAT_CLOSE:
        Window_Destroy((Window*)pFloatingWindowContainer);
        break;
    }
}

LRESULT FloatingWindowContainer_OnNCLButtonDown(FloatingWindowContainer* pFloatingWindowContainer, UINT hitTestVal, int x, int y)
{
    DWORD dwStyle = Window_GetStyle((Window *)pFloatingWindowContainer);

    if (hitTestVal == HTCLOSE)
    {
        Window_Destroy((Window*)pFloatingWindowContainer);
        return TRUE;
    }
    else if (hitTestVal == HTMAXBUTTON)
    {
        if (IsZoomed(pFloatingWindowContainer->base.hWnd))
        {
            ShowWindow(pFloatingWindowContainer->base.hWnd, SW_RESTORE);
        }
        else {
            ShowWindow(pFloatingWindowContainer->base.hWnd, SW_MAXIMIZE);
        }
        return TRUE;
    }
    else if (hitTestVal == HTMINBUTTON)
    {
        ShowWindow(pFloatingWindowContainer->base.hWnd, SW_MINIMIZE);
        return TRUE;
    }
    else if (hitTestVal == HTMORE)
    {
        POINT ptScreen = { x, y };
        FloatingWindowContainer_ShowPanelMenu(pFloatingWindowContainer, ptScreen);
        return TRUE;
    }
    else if (dwStyle & WS_CHILD && hitTestVal == HTCAPTION)
    {
        // SetCapture(hWnd);
        pFloatingWindowContainer->fCaptionUnpinStarted = TRUE;
        pFloatingWindowContainer->ptCaptionUnpinStartingPoint.x = x;
        pFloatingWindowContainer->ptCaptionUnpinStartingPoint.y = y;
        return TRUE;
    }

    switch (hitTestVal)
    {
    case HTCLOSE:
    case HTMINBUTTON:
    case HTMAXBUTTON:
        return TRUE;
    }

    return FALSE;
}

void FloatingWindowContainer_OnUnpinCommand(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (pFloatingWindowContainer->bPinned)
    {
        HWND hWndPrevParent = SetParent(pFloatingWindowContainer->base.hWnd, NULL);
        pFloatingWindowContainer->hWndPrevParent = hWndPrevParent;
        
        DWORD dwStyle = Window_GetStyle((Window*)pFloatingWindowContainer);
        dwStyle &= ~WS_CHILD;
        dwStyle |= WS_OVERLAPPED | WS_THICKFRAME;
        Window_SetStyle((Window*)pFloatingWindowContainer, dwStyle);

        pFloatingWindowContainer->bPinned = FALSE;
        return;
    }

    ASSERT(pFloatingWindowContainer->hWndPrevParent);
    SetParent(pFloatingWindowContainer->base.hWnd, pFloatingWindowContainer->hWndPrevParent);

    DWORD dwStyle = Window_GetStyle((Window*)pFloatingWindowContainer);
    dwStyle &= ~(WS_OVERLAPPED | WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    Window_SetStyle((Window*)pFloatingWindowContainer, dwStyle);

    pFloatingWindowContainer->bPinned = TRUE;    
}

void FloatingWindowContainer_OnCloseCommand(FloatingWindowContainer* pFloatingWindowContainer)
{
    Window_Destroy((Window*)pFloatingWindowContainer);
}

LRESULT FloatingWindowContainer_OnNCMouseMove(FloatingWindowContainer* pFloatingWindowContainer, UINT hitTestVal, int x, int y)
{
    UNREFERENCED_PARAMETER(hitTestVal);

    if (pFloatingWindowContainer->fCaptionUnpinStarted)
    {
        const int xStart = pFloatingWindowContainer->ptCaptionUnpinStartingPoint.x;
        const int yStart = pFloatingWindowContainer->ptCaptionUnpinStartingPoint.y;

        HWND hWnd = Window_GetHWND((Window*)pFloatingWindowContainer);

        HDC hdcDesktop = GetDC(NULL);
        HDC hdcScreenshot = CreateCompatibleDC(hdcDesktop);

        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
        int width = Win32_Rect_GetWidth(&rcWindow);
        int height = Win32_Rect_GetHeight(&rcWindow);
        HBITMAP hbmScreenshot = CreateCompatibleBitmap(hdcDesktop, width, height);

        SelectObject(hdcScreenshot, hbmScreenshot);
        BOOL bResult = PrintWindow(hWnd, hdcScreenshot, 0);
        UNREFERENCED_PARAMETER(bResult);

        POINT pt;
        pt.x = x;
        pt.y = y;
        MapWindowPoints(NULL, hWnd, &pt, 1);

        BitBlt(hdcDesktop, x - pt.x, y + pt.y, width, height, hdcScreenshot, 0, 0, SRCCOPY);


        if (sqrt(pow(x - xStart, 2) + pow(y - yStart, 2)) >= 100)
        {
            // ReleaseCapture();
            FloatingWindowContainer_OnUnpinCommand(pFloatingWindowContainer);
            pFloatingWindowContainer->fCaptionUnpinStarted = FALSE;
        }

        DeleteObject(hbmScreenshot);
        DeleteDC(hdcScreenshot);
        ReleaseDC(NULL, hdcDesktop);

        return TRUE;
    }

    return FALSE;
}

LRESULT FloatingWindowContainer_OnNCLButtonUp(FloatingWindowContainer* pFloatingWindowContainer, UINT hitTestVal, int x, int y)
{
    UNREFERENCED_PARAMETER(hitTestVal);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (pFloatingWindowContainer->fCaptionUnpinStarted)
    {
        ReleaseCapture();
        pFloatingWindowContainer->fCaptionUnpinStarted = FALSE;
    }

    return FALSE;
}

LRESULT FloatingWindowContainer_OnCommand(FloatingWindowContainer* pFloatingWindowContainer, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (LOWORD(wParam))
    {
    case IDM_FLOAT_DOCK:
        if (FloatingWindowContainer_DockByCenter(pFloatingWindowContainer))
        {
            DestroyWindow(pFloatingWindowContainer->base.hWnd);
        }
        return 0;

    case IDM_FLOAT_CLOSE:
        FloatingWindowContainer_OnCloseCommand(pFloatingWindowContainer);
        return 0;

    case IDM_FLOAT_AUTOHIDE:
    case IDM_FLOAT_MOVETONEW:
        return 0;
    }

    return FALSE;
}

LRESULT FloatingWindowContainer_OnNCActivate(FloatingWindowContainer* pFloatingWindowContainer, BOOL fActive)
{
    UNREFERENCED_PARAMETER(fActive);

    DWORD dwStyle = Window_GetStyle((Window *)pFloatingWindowContainer);

    // Prevent drawing default 5px gray thick frame
    if (dwStyle & WS_THICKFRAME)
    {
        return TRUE;
    }

    return FALSE;
}

void FloatingWindowContainer_OnSize(FloatingWindowContainer* pFloatingWindowContainer, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(state);

    if (pFloatingWindowContainer->hWndChild && IsWindow(pFloatingWindowContainer->hWndChild))
    {
        SetWindowPos(pFloatingWindowContainer->hWndChild, NULL, 0, 0, cx, cy, SWP_NOREDRAW);
    }
}

static BOOL FloatingWindowContainer_AttemptDockDocument(FloatingWindowContainer* pFloatingWindowContainer, BOOL bForceMainWorkspace)
{
    if (!pFloatingWindowContainer)
    {
        return FALSE;
    }

    WorkspaceContainer* pWorkspaceSource = FloatingWindowContainer_GetWorkspaceChild(pFloatingWindowContainer);
    if (!pWorkspaceSource)
    {
        return FALSE;
    }

    WorkspaceContainer* pWorkspaceTarget = NULL;
    if (bForceMainWorkspace)
    {
        pWorkspaceTarget = PanitentApp_GetWorkspaceContainer(PanitentApp_Instance());
    }
    else {
        POINT ptCursor = { 0 };
        if (!GetCursorPos(&ptCursor))
        {
            return FALSE;
        }

        pWorkspaceTarget = WorkspaceContainer_FindDropTargetAtScreenPoint(pWorkspaceSource, ptCursor);
    }

    if (!pWorkspaceTarget || pWorkspaceTarget == pWorkspaceSource)
    {
        return FALSE;
    }

    WorkspaceContainer_MoveAllViewportsTo(pWorkspaceSource, pWorkspaceTarget);
    return TRUE;
}

static BOOL FloatingWindowContainer_AttemptDock(FloatingWindowContainer* pFloatingWindowContainer)
{
    if (pFloatingWindowContainer && pFloatingWindowContainer->nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT)
    {
        return FloatingWindowContainer_AttemptDockDocument(pFloatingWindowContainer, FALSE);
    }

    if (!pFloatingWindowContainer ||
        !pFloatingWindowContainer->pDockHostTarget ||
        !pFloatingWindowContainer->hWndChild ||
        !IsWindow(pFloatingWindowContainer->hWndChild))
    {
        return FALSE;
    }

    POINT ptCursor = { 0 };
	if (!GetCursorPos(&ptCursor))
	{
		return FALSE;
	}

	DockTargetHit dockTarget = { 0 };
	if (!DockHostWindow_HitTestDockTarget(pFloatingWindowContainer->pDockHostTarget, ptCursor, &dockTarget) ||
		dockTarget.nDockSide == DKS_NONE)
	{
		return FALSE;
	}
	int nDockSide = dockTarget.nDockSide;

    RECT rcFloating = { 0 };
    GetWindowRect(Window_GetHWND((Window*)pFloatingWindowContainer), &rcFloating);

    int iDockSize = pFloatingWindowContainer->iDockSizeHint;
    if (nDockSide == DKS_LEFT || nDockSide == DKS_RIGHT)
    {
        iDockSize = max(iDockSize, Win32_Rect_GetWidth(&rcFloating));
    }
    else {
        iDockSize = max(iDockSize, Win32_Rect_GetHeight(&rcFloating));
    }

    iDockSize = max(iDockSize, 180);

	HWND hWndChild = pFloatingWindowContainer->hWndChild;
	pFloatingWindowContainer->hWndChild = NULL;

	if (!DockHostWindow_DockHWNDToTarget(pFloatingWindowContainer->pDockHostTarget, hWndChild, &dockTarget, iDockSize))
	{
		pFloatingWindowContainer->hWndChild = hWndChild;
		return FALSE;
    }

    return TRUE;
}

LRESULT CALLBACK FloatingWindowContainer_UserProc(FloatingWindowContainer* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_NCCREATE:
        return FloatingWindowContainer_OnNCCreate(window, (LPCREATESTRUCT)lParam);
        break;

    case WM_NCPAINT:
        FloatingWindowContainer_OnNCPaint(window, (HRGN)wParam);
        return 0;
        break;

    case WM_NCCALCSIZE:
        return FloatingWindowContainer_OnNCCalcSize(window, (BOOL)wParam ? TRUE : FALSE, lParam);
        break;

    case WM_NCHITTEST:
        return FloatingWindowContainer_OnNCHitTest(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_NCMOUSEMOVE:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCMouseMove(window, (UINT)wParam, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
    break;

    case WM_NCLBUTTONDOWN:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCLButtonDown(window, (UINT)wParam, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
        break;

    case WM_NCLBUTTONUP:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCLButtonUp(window, (UINT)wParam, (int)GET_X_LPARAM(lParam), (int)GET_Y_LPARAM(lParam));
        if (bProcessed)
        {
            return 0;
        }
    }
    break;

    case WM_NCACTIVATE:
    {
        BOOL bProcessed = (BOOL)FloatingWindowContainer_OnNCActivate(window, (BOOL)wParam);
        if (bProcessed)
        {
            return TRUE;
        }
    }
        break;

    case WM_MOVING:
    case WM_SIZING:
        FloatingWindowContainer_UpdateDockPreviewOverlay(window);
        break;

    case WM_EXITSIZEMOVE:
    {
        FloatingWindowContainer_DestroyDockPreviewOverlay();
        if (FloatingWindowContainer_AttemptDock(window))
        {
            DestroyWindow(window->base.hWnd);
            return 0;
        }
    }
        break;

    case WM_COMMAND:
        return FloatingWindowContainer_OnCommand(window, wParam, lParam);
        break;

    case WM_RBUTTONUP:
        FloatingWindowContainer_OnRButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        FloatingWindowContainer_OnLButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        FloatingWindowContainer_OnContextMenu(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault((Window*)window, hWnd, message, wParam, lParam);
}

void FloatingWindowContainer_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"FloatingWindowContainer";
    lpcs->style = WS_CAPTION | WS_OVERLAPPED | WS_THICKFRAME | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

void FloatingWindowContainer_PinWindow(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndChild)
{
    if (hWndChild && IsWindow(hWndChild))
    {
        HWND hWnd = Window_GetHWND((Window *)pFloatingWindowContainer);

        RECT rcChildWindow = { 0 };
        GetWindowRect(hWndChild, &rcChildWindow);
        
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        HWND hWndPrevParent = SetParent(hWndChild, hWnd);
        pFloatingWindowContainer->hWndPrevParent = hWndPrevParent;

        DWORD dwStyle = GetWindowStyle(hWndChild);
        dwStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_POPUP);
        dwStyle |= WS_CHILD;
        SetWindowLongPtr(hWndChild, GWL_STYLE, dwStyle);
        SetWindowPos(hWndChild, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        
        pFloatingWindowContainer->hWndChild = hWndChild;
        pFloatingWindowContainer->bPinned = TRUE;
        pFloatingWindowContainer->iDockSizeHint = max(Win32_Rect_GetWidth(&rcChildWindow), 220);

        SetWindowPos(hWndChild, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        UpdateWindow(hWnd);
    }
}

void FloatingWindowContainer_SetDockTarget(FloatingWindowContainer* pFloatingWindowContainer, DockHostWindow* pDockHostWindow)
{
    if (!pFloatingWindowContainer)
    {
        return;
    }

    pFloatingWindowContainer->pDockHostTarget = pDockHostWindow;
}

void FloatingWindowContainer_SetDockPolicy(FloatingWindowContainer* pFloatingWindowContainer, int nDockPolicy)
{
    if (!pFloatingWindowContainer)
    {
        return;
    }

    if (pFloatingWindowContainer->nDockPolicy == nDockPolicy)
    {
        return;
    }

    pFloatingWindowContainer->nDockPolicy = nDockPolicy;

    HWND hWnd = Window_GetHWND((Window*)pFloatingWindowContainer);
    if (hWnd && IsWindow(hWnd))
    {
        SetWindowPos(
            hWnd,
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

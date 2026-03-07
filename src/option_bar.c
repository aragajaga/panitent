#include "precomp.h"
#include "win32/window.h"
#include "option_bar.h"

#include "win32/util.h"

#include <commctrl.h>
#include <windowsx.h>
#include "swatch.h"
#include "brush.h"
#include <assert.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include "resource.h"
#include <math.h>
#include <strsafe.h>

#include "panitentapp.h"
#include "tool.h"
#include "grimstroke/shapecontext.h"
#include "grimstroke/shapestrategy.h"
#include "grimstroke/bresenhamshapestrategy.h"
#include "grimstroke/wushapestrategy.h"

static const WCHAR szClassName[] = L"__OptionBarWindow";

#define IDCB_STENCIL_ALGORITHM 1553
#define IDCB_THICKNESS 1554
#define IDB_SHAPESTROKE 1555
#define IDB_SHAPEFILL 1556
#define IDCB_FONT_FAMILY 1557
#define IDCB_TEXT_SIZE 1558
#define WM_OPTIONBAR_REFRESH_STATE (WM_APP + 41)

enum {
	OPTIONBAR_MODE_NONE = 0,
	OPTIONBAR_MODE_SHAPE,
	OPTIONBAR_MODE_LINE,
	OPTIONBAR_MODE_BRUSH,
	OPTIONBAR_MODE_TEXT
};

static const int kOptionBarPadding = 8;
static const int kOptionBarControlTop = 4;
static const int kOptionBarControlHeight = 22;
static const int kOptionBarStrokeWidth = 66;
static const int kOptionBarFillWidth = 54;
static const int kOptionBarAlgorithmWidth = 112;
static const int kOptionBarThicknessWidth = 72;
static const int kOptionBarBrushWidth = 64;
static const int kOptionBarFontWidth = 220;
static const int kOptionBarTextSizeWidth = 72;
static const int kOptionBarControlGap = 6;
static const int kOptionBarTextSizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 30, 36, 48, 72 };

typedef struct OptionBarFontEnumContext {
	HWND hWndCombo;
} OptionBarFontEnumContext;

static BOOL OptionBarWindow_IsToolLabel(const Tool* pTool, PCWSTR pszLabel);
static int OptionBarWindow_GetModeForTool(Tool* pTool);
static void OptionBarWindow_SetControlVisible(HWND hWnd, BOOL fVisible);
static void OptionBarWindow_MoveVisibleControl(HWND hWnd, int* px, int y, int width, BOOL fCombo);
static BOOL OptionBarWindow_EnsureComboText(HWND hWndCombo, PCWSTR pszText);
static void OptionBarWindow_SelectComboText(HWND hWndCombo, PCWSTR pszText);
static void OptionBarWindow_SelectComboInt(HWND hWndCombo, int nValue);
static int OptionBarWindow_GetComboIntValue(HWND hWndCombo, int nFallback);
static int CALLBACK OptionBarWindow_EnumFontFamProc(const LOGFONTW* plf, const TEXTMETRICW* ptm, DWORD fontType, LPARAM lParam);
static void OptionBarWindow_PopulateFontCombo(HWND hWndCombo);
static void OptionBarWindow_PopulateTextSizeCombo(HWND hWndCombo);
static int OptionBarWindow_GetStrategySelection(ShapeContext* pShapeContext);
static void OptionBarWindow_RefreshShapeState(OptionBarWindow* pOptionBarWindow);
static void OptionBarWindow_RefreshBrushState(OptionBarWindow* pOptionBarWindow);
static void OptionBarWindow_RefreshTextState(OptionBarWindow* pOptionBarWindow);

static ShapeStrategy* OptionBarWindow_GetStrategyForSelection(int selection)
{
	static BresenhamShapeStrategy* s_pBresenhamStrategy = NULL;
	static WuShapeStrategy* s_pWuShapeStrategy = NULL;

	switch (selection)
	{
	case 0:
		if (!s_pBresenhamStrategy)
		{
			s_pBresenhamStrategy = BresenhamShapeStrategy_Create();
		}
		return (ShapeStrategy*)s_pBresenhamStrategy;
	case 1:
	default:
		if (!s_pWuShapeStrategy)
		{
			s_pWuShapeStrategy = WuShapeStrategy_Create();
		}
		return (ShapeStrategy*)s_pWuShapeStrategy;
	}
}

static BOOL OptionBarWindow_IsToolLabel(const Tool* pTool, PCWSTR pszLabel)
{
	return pTool && pTool->pszLabel && pszLabel && wcscmp(pTool->pszLabel, pszLabel) == 0;
}

static int OptionBarWindow_GetModeForTool(Tool* pTool)
{
	if (OptionBarWindow_IsToolLabel(pTool, L"Rectangle") || OptionBarWindow_IsToolLabel(pTool, L"Circle"))
	{
		return OPTIONBAR_MODE_SHAPE;
	}
	if (OptionBarWindow_IsToolLabel(pTool, L"Line"))
	{
		return OPTIONBAR_MODE_LINE;
	}
	if (OptionBarWindow_IsToolLabel(pTool, L"Brush") || OptionBarWindow_IsToolLabel(pTool, L"Eraser"))
	{
		return OPTIONBAR_MODE_BRUSH;
	}
	if (OptionBarWindow_IsToolLabel(pTool, L"Text"))
	{
		return OPTIONBAR_MODE_TEXT;
	}

	return OPTIONBAR_MODE_NONE;
}

static void OptionBarWindow_SetControlVisible(HWND hWnd, BOOL fVisible)
{
	if (hWnd)
	{
		ShowWindow(hWnd, fVisible ? SW_SHOW : SW_HIDE);
	}
}

static void OptionBarWindow_MoveVisibleControl(HWND hWnd, int* px, int y, int width, BOOL fCombo)
{
	if (!hWnd || !px || !IsWindowVisible(hWnd))
	{
		return;
	}

	MoveWindow(
		hWnd,
		*px,
		y + (fCombo ? 0 : 1),
		width,
		kOptionBarControlHeight + (fCombo ? 200 : 0),
		TRUE);
	*px += width + kOptionBarControlGap;
}

static BOOL OptionBarWindow_EnsureComboText(HWND hWndCombo, PCWSTR pszText)
{
	if (!hWndCombo || !pszText || pszText[0] == L'\0')
	{
		return FALSE;
	}

	if (ComboBox_FindStringExact(hWndCombo, -1, pszText) == CB_ERR)
	{
		return ComboBox_AddString(hWndCombo, pszText) != CB_ERR;
	}

	return TRUE;
}

static void OptionBarWindow_SelectComboText(HWND hWndCombo, PCWSTR pszText)
{
	if (!hWndCombo || !pszText || pszText[0] == L'\0')
	{
		return;
	}

	if (!OptionBarWindow_EnsureComboText(hWndCombo, pszText))
	{
		return;
	}

	int index = ComboBox_FindStringExact(hWndCombo, -1, pszText);
	if (index != CB_ERR)
	{
		ComboBox_SetCurSel(hWndCombo, index);
	}
}

static void OptionBarWindow_SelectComboInt(HWND hWndCombo, int nValue)
{
	WCHAR szValue[16] = L"";
	_itow_s(max(1, nValue), szValue, ARRAYSIZE(szValue), 10);
	OptionBarWindow_SelectComboText(hWndCombo, szValue);
}

static int OptionBarWindow_GetComboIntValue(HWND hWndCombo, int nFallback)
{
	WCHAR szValue[32] = L"";
	if (!hWndCombo || GetWindowTextW(hWndCombo, szValue, ARRAYSIZE(szValue)) <= 0)
	{
		return max(1, nFallback);
	}

	int nValue = _wtoi(szValue);
	return max(1, nValue > 0 ? nValue : nFallback);
}

static int CALLBACK OptionBarWindow_EnumFontFamProc(const LOGFONTW* plf, const TEXTMETRICW* ptm, DWORD fontType, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(ptm);
	UNREFERENCED_PARAMETER(fontType);

	OptionBarFontEnumContext* pContext = (OptionBarFontEnumContext*)lParam;
	if (!pContext || !pContext->hWndCombo || !plf || plf->lfFaceName[0] == L'\0' || plf->lfFaceName[0] == L'@')
	{
		return 1;
	}

	OptionBarWindow_EnsureComboText(pContext->hWndCombo, plf->lfFaceName);
	return 1;
}

static void OptionBarWindow_PopulateFontCombo(HWND hWndCombo)
{
	if (!hWndCombo)
	{
		return;
	}

	HDC hdc = GetDC(hWndCombo);
	if (!hdc)
	{
		return;
	}

	LOGFONTW lf = { 0 };
	lf.lfCharSet = DEFAULT_CHARSET;

	OptionBarFontEnumContext context = { hWndCombo };
	EnumFontFamiliesExW(hdc, &lf, OptionBarWindow_EnumFontFamProc, (LPARAM)&context, 0);
	ReleaseDC(hWndCombo, hdc);
}

static void OptionBarWindow_PopulateTextSizeCombo(HWND hWndCombo)
{
	if (!hWndCombo)
	{
		return;
	}

	for (size_t i = 0; i < ARRAYSIZE(kOptionBarTextSizes); ++i)
	{
		WCHAR szValue[16] = L"";
		_itow_s(kOptionBarTextSizes[i], szValue, ARRAYSIZE(szValue), 10);
		OptionBarWindow_EnsureComboText(hWndCombo, szValue);
	}
}

static int OptionBarWindow_GetStrategySelection(ShapeContext* pShapeContext)
{
	ShapeStrategy* pStrategy = pShapeContext ? ShapeContext_GetStrategy(pShapeContext) : NULL;
	ShapeStrategy* pBresenham = OptionBarWindow_GetStrategyForSelection(0);

	if (pStrategy && pBresenham &&
		pStrategy->DrawLine == pBresenham->DrawLine &&
		pStrategy->DrawCircle == pBresenham->DrawCircle)
	{
		return 0;
	}

	return 1;
}

static void OptionBarWindow_RefreshShapeState(OptionBarWindow* pOptionBarWindow)
{
	ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
	if (!pOptionBarWindow || !pShapeContext)
	{
		return;
	}

	Button_SetCheck(
		pOptionBarWindow->hWndStrokeCheck,
		ShapeContext_IsStrokeEnabled(pShapeContext) ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(
		pOptionBarWindow->hWndFillCheck,
		ShapeContext_IsFillEnabled(pShapeContext) ? BST_CHECKED : BST_UNCHECKED);
	ComboBox_SetCurSel(
		pOptionBarWindow->hWndAlgorithmCombo,
		OptionBarWindow_GetStrategySelection(pShapeContext));
	OptionBarWindow_SelectComboInt(
		pOptionBarWindow->hWndThicknessCombo,
		ShapeContext_GetStrokeWidth(pShapeContext));
}

static void OptionBarWindow_RefreshBrushState(OptionBarWindow* pOptionBarWindow)
{
	if (!pOptionBarWindow)
	{
		return;
	}

	InitializeBrushList();
	OptionBarWindow_SelectComboInt(pOptionBarWindow->hWndThicknessCombo, g_brushSize);
	if (pOptionBarWindow->hWndBrushSelector)
	{
		InvalidateRect(pOptionBarWindow->hWndBrushSelector, NULL, FALSE);
	}
}

static void OptionBarWindow_RefreshTextState(OptionBarWindow* pOptionBarWindow)
{
	PanitentApp* pPanitentApp = PanitentApp_Instance();
	if (!pOptionBarWindow || !pPanitentApp)
	{
		return;
	}

	OptionBarWindow_SelectComboText(
		pOptionBarWindow->hWndFontCombo,
		PanitentApp_GetTextToolFontFace(pPanitentApp));
	OptionBarWindow_SelectComboInt(
		pOptionBarWindow->hWndTextSizeCombo,
		PanitentApp_GetTextToolFontPx(pPanitentApp));
}

INT_PTR CALLBACK BrushProp_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern BrushBuilder g_brushList[80];

WCHAR szBrushSelClass[] = L"Win32Class_BrushSel";
static HTHEME hTheme = NULL;

void BrushSel_OnCreate(HWND hwnd, LPCREATESTRUCT lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	InitializeBrushList();
	hTheme = OpenThemeData(hwnd, L"ComboBox");
}

void BrushSel_OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps = { 0 };
	HDC hdc = BeginPaint(hwnd, &ps);
	HGDIOBJ hOldObj;

	int brushSize = min(g_brushSize, 24);

	if (brushSize)
	{
		/* Gather active application brush */
		BrushBuilder* builder = g_pBrush;
		Brush* brush = BrushBuilder_Build(builder, brushSize);
		Canvas* tex = Brush_GetTexture(brush);

		if (tex)
		{
			/* Copy texture into GDI bitmap */
			BITMAPINFO bmi;
			Win32_InitGdiBitmapInfo32Bpp(&bmi, 24, 24);

			uint32_t* buffer = NULL;

			HDC hSampleDC = CreateCompatibleDC(hdc);
			HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (LPVOID*)&buffer, NULL, 0);
			assert(hBitmap != NULL);
			assert(buffer != NULL);

			hOldObj = SelectObject(hSampleDC, hBitmap);
			RECT rc = { 0, 0, 24, 24 };
			FillRect(hSampleDC, &rc, GetStockObject(WHITE_BRUSH));

			int offset = (24 - brushSize) / 2;
			for (int y = 0; y < brushSize; y++)
			{
				uint32_t* pDst = buffer + (size_t)(offset + y) * 24 + (size_t)offset;
				uint32_t* pSrc = ((uint32_t*)tex->buffer) + (size_t)y * (size_t)tex->width;
				for (int x = 0; x < brushSize; x++)
				{
					pDst[x] = mix(0xFFFFFFFF, pSrc[x]);
				}
			}

			/* Blit brush sample */
			BitBlt(hdc, 0, 0, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight, hSampleDC,
				0, 0, SRCCOPY);

			SelectObject(hSampleDC, hOldObj);

			DeleteObject(hBitmap);
			DeleteDC(hSampleDC);
		}

		Brush_Delete(brush);
	}


	/* Draw size label */
	RECT clientRc = { 0 };
	GetClientRect(hwnd, &clientRc);

	HFONT hFont = PanitentApp_GetUIFont(PanitentApp_Instance());
	hOldObj = SelectObject(hdc, hFont);

	WCHAR szBrushSize[4];
	_itow_s(g_brushSize, szBrushSize, 4, 10);
	int cchBrushSize = lstrlenW(szBrushSize);

	SIZE sText;
	GetTextExtentPoint32(hdc, szBrushSize, cchBrushSize, &sText);

	TextOut(hdc, 26, (clientRc.bottom - sText.cy) / 2, szBrushSize, cchBrushSize);

	SelectObject(hdc, hOldObj);

	/* Dropdown button */
	RECT dropBtnRc = clientRc;
	dropBtnRc.left = dropBtnRc.right - 16;

	if (hTheme)
	{
		DrawThemeBackground(hTheme, hdc, CP_DROPDOWNBUTTON, CBXS_NORMAL,
			&dropBtnRc, NULL);
	}
	else {
		DrawFrameControl(hdc, &dropBtnRc, DFC_SCROLL, DFCS_SCROLLCOMBOBOX);
	}

	EndPaint(hwnd, &ps);
}

void BrushSel_OnDestroy(HWND hWnd)
{
	UNREFERENCED_PARAMETER(hWnd);

	CloseThemeData(hTheme);
}

void BrushSel_OnLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_BRUSHPROP), hwnd,
		(DLGPROC)BrushProp_DlgProc);

	InvalidateRect(hwnd, NULL, FALSE);
	SendMessage(GetParent(hwnd), WM_OPTIONBAR_REFRESH_STATE, 0, 0);
}

LRESULT CALLBACK BrushSel_WndProc(HWND hwnd, UINT message, WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		BrushSel_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
		return 0;
	case WM_PAINT:
		BrushSel_OnPaint(hwnd);
		return 0;
	case WM_LBUTTONDOWN:
		BrushSel_OnLButtonDown(hwnd, wParam, lParam);
		return 0;
	case WM_DESTROY:
		BrushSel_OnDestroy(hwnd);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL BrushSel_RegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcexExisting = { 0 };
	if (GetClassInfoEx(hInstance, szBrushSelClass, &wcexExisting))
	{
		return TRUE;
	}

	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)BrushSel_WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szBrushSelClass;
	wcex.hIconSm = NULL;
	return RegisterClassEx(&wcex);
}

static void OptionBarWindow_LayoutControls(OptionBarWindow* pOptionBarWindow, int cx, int cy)
{
	UNREFERENCED_PARAMETER(cy);

	if (!pOptionBarWindow)
	{
		return;
	}

	int y = kOptionBarControlTop;
	int x = kOptionBarPadding;
	int availableWidth = max(0, cx - kOptionBarPadding * 2);

	if (pOptionBarWindow->nMode == OPTIONBAR_MODE_TEXT)
	{
		int fontWidth = min(kOptionBarFontWidth, max(120, availableWidth - kOptionBarTextSizeWidth - kOptionBarControlGap));
		OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndFontCombo, &x, y, fontWidth, TRUE);
		OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndTextSizeCombo, &x, y, kOptionBarTextSizeWidth, TRUE);
		return;
	}

	OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndStrokeCheck, &x, y, kOptionBarStrokeWidth, FALSE);
	OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndFillCheck, &x, y, kOptionBarFillWidth, FALSE);
	OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndAlgorithmCombo, &x, y, kOptionBarAlgorithmWidth, TRUE);
	OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndThicknessCombo, &x, y, kOptionBarThicknessWidth, TRUE);
	OptionBarWindow_MoveVisibleControl(pOptionBarWindow->hWndBrushSelector, &x, y, kOptionBarBrushWidth, FALSE);
}

void OptionBarWindow_SyncTool(OptionBarWindow* pOptionBarWindow, Tool* pTool)
{
	if (!pOptionBarWindow)
	{
		return;
	}

	int nMode = OptionBarWindow_GetModeForTool(pTool);
	pOptionBarWindow->nMode = nMode;
	pOptionBarWindow->fShowFill = (nMode == OPTIONBAR_MODE_SHAPE);

	if (nMode == OPTIONBAR_MODE_LINE)
	{
		ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());
		if (pShapeContext)
		{
			ShapeContext_SetStrokeEnabled(pShapeContext, TRUE);
		}
	}

	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndStrokeCheck, nMode == OPTIONBAR_MODE_SHAPE);
	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndFillCheck, nMode == OPTIONBAR_MODE_SHAPE && pOptionBarWindow->fShowFill);
	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndAlgorithmCombo, nMode == OPTIONBAR_MODE_SHAPE || nMode == OPTIONBAR_MODE_LINE);
	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndThicknessCombo, nMode == OPTIONBAR_MODE_SHAPE || nMode == OPTIONBAR_MODE_LINE || nMode == OPTIONBAR_MODE_BRUSH);
	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndBrushSelector, nMode == OPTIONBAR_MODE_BRUSH);
	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndFontCombo, nMode == OPTIONBAR_MODE_TEXT);
	OptionBarWindow_SetControlVisible(pOptionBarWindow->hWndTextSizeCombo, nMode == OPTIONBAR_MODE_TEXT);

	switch (nMode)
	{
	case OPTIONBAR_MODE_SHAPE:
	case OPTIONBAR_MODE_LINE:
		OptionBarWindow_RefreshShapeState(pOptionBarWindow);
		break;
	case OPTIONBAR_MODE_BRUSH:
		OptionBarWindow_RefreshBrushState(pOptionBarWindow);
		break;
	case OPTIONBAR_MODE_TEXT:
		OptionBarWindow_RefreshTextState(pOptionBarWindow);
		break;
	}

	HWND hWnd = Window_GetHWND((Window*)pOptionBarWindow);
	if (hWnd)
	{
		RECT rcClient = { 0 };
		GetClientRect(hWnd, &rcClient);
		OptionBarWindow_LayoutControls(pOptionBarWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
		InvalidateRect(hWnd, NULL, FALSE);
	}
}

LRESULT OptionBarWindow_OnCommand(OptionBarWindow* pOptionBarWindow, WPARAM wparam, LPARAM lparam)
{
		ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());

		if (HIWORD(wparam) == BN_CLICKED)
		{
			switch (LOWORD(wparam))
			{
			case IDB_SHAPESTROKE:
				if (pShapeContext)
				{
					ShapeContext_SetStrokeEnabled(pShapeContext, Button_GetCheck((HWND)lparam) == BST_CHECKED);
				}
				return 0;
			case IDB_SHAPEFILL:
				if (pShapeContext)
				{
					ShapeContext_SetFillEnabled(pShapeContext, Button_GetCheck((HWND)lparam) == BST_CHECKED);
				}
				return 0;
			}
		}

		if (HIWORD(wparam) != CBN_SELCHANGE)
		{
			return 0;
		}

		switch (LOWORD(wparam))
		{
		case IDCB_STENCIL_ALGORITHM:
		{
			ShapeStrategy* pStrategy = OptionBarWindow_GetStrategyForSelection(ComboBox_GetCurSel((HWND)lparam));
			if (pShapeContext && pStrategy)
			{
				ShapeContext_SetStrategy(pShapeContext, pStrategy);
			}
		}
		break;
		case IDCB_THICKNESS:
		{
			int nValue = OptionBarWindow_GetComboIntValue((HWND)lparam, 1);
			if (pOptionBarWindow->nMode == OPTIONBAR_MODE_BRUSH)
			{
				g_brushSize = max(1, nValue);
				if (pOptionBarWindow->hWndBrushSelector)
				{
					InvalidateRect(pOptionBarWindow->hWndBrushSelector, NULL, FALSE);
				}
			}
			else if (pShapeContext)
			{
				ShapeContext_SetStrokeWidth(pShapeContext, nValue);
			}
		}
			break;
		case IDCB_FONT_FAMILY:
		{
			WCHAR szFaceName[LF_FACESIZE] = L"";
			GetWindowTextW((HWND)lparam, szFaceName, ARRAYSIZE(szFaceName));
			PanitentApp_SetTextToolFontFace(PanitentApp_Instance(), szFaceName);
		}
			break;
		case IDCB_TEXT_SIZE:
			PanitentApp_SetTextToolFontPx(
				PanitentApp_Instance(),
				OptionBarWindow_GetComboIntValue((HWND)lparam, PanitentApp_GetTextToolFontPx(PanitentApp_Instance())));
			break;
		}

		return 0;
}

BOOL OptionBarWindow_OnCreate(OptionBarWindow* pOptionBarWindow, LPCREATESTRUCT lpcs)
{
	UNREFERENCED_PARAMETER(lpcs);

	HWND hWnd = Window_GetHWND((Window *)pOptionBarWindow);
	HINSTANCE hInstance = GetModuleHandle(NULL);

	BrushSel_RegisterClass(hInstance);

	HWND hStrokeCheck = CreateWindowEx(
		0,
		WC_BUTTON,
		L"Stroke",
		BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
		0,
		0,
		0,
		0,
		hWnd,
		(HMENU)(INT_PTR)IDB_SHAPESTROKE,
		hInstance,
		NULL);
	Win32_ApplyUIFont(hStrokeCheck);
	Button_SetCheck(hStrokeCheck, BST_CHECKED);
	pOptionBarWindow->hWndStrokeCheck = hStrokeCheck;

	HWND hFillCheck = CreateWindowEx(
		0,
		WC_BUTTON,
		L"Fill",
		BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
		0,
		0,
		0,
		0,
		hWnd,
		(HMENU)(INT_PTR)IDB_SHAPEFILL,
		hInstance,
		NULL);
	Win32_ApplyUIFont(hFillCheck);
	pOptionBarWindow->hWndFillCheck = hFillCheck;

	HWND hAlgorithmCombo = CreateWindowEx(
		0,
		WC_COMBOBOX,
		L"",
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0,
		0,
		0,
		0,
		hWnd,
		(HMENU)(INT_PTR)IDCB_STENCIL_ALGORITHM,
		hInstance,
		NULL);
		Win32_ApplyUIFont(hAlgorithmCombo);
		ComboBox_AddString(hAlgorithmCombo, L"Bresenham");
		ComboBox_AddString(hAlgorithmCombo, L"Wu");
		ComboBox_SetCurSel(hAlgorithmCombo, 1);
		pOptionBarWindow->hWndAlgorithmCombo = hAlgorithmCombo;

	HWND hThicknessCombo = CreateWindowEx(
		0,
		WC_COMBOBOX,
		L"",
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0,
		0,
		0,
		0,
		hWnd,
		(HMENU)(INT_PTR)IDCB_THICKNESS,
		hInstance,
		NULL);
	Win32_ApplyUIFont(hThicknessCombo);
	for (int i = 1; i <= 32; ++i)
	{
		WCHAR szThickness[16] = L"";
		_itow_s(i, szThickness, ARRAYSIZE(szThickness), 10);
		ComboBox_AddString(hThicknessCombo, szThickness);
	}
	ComboBox_SetCurSel(hThicknessCombo, 0);
	pOptionBarWindow->hWndThicknessCombo = hThicknessCombo;

	HWND hBrushSelector = CreateWindowEx(0, szBrushSelClass, NULL,
		WS_BORDER | WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		hWnd, NULL, hInstance, NULL);
	pOptionBarWindow->hWndBrushSelector = hBrushSelector;

	HWND hFontCombo = CreateWindowEx(
		0,
		WC_COMBOBOX,
		L"",
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_SORT | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0,
		0,
		0,
		0,
		hWnd,
		(HMENU)(INT_PTR)IDCB_FONT_FAMILY,
		hInstance,
		NULL);
	Win32_ApplyUIFont(hFontCombo);
	OptionBarWindow_PopulateFontCombo(hFontCombo);
	pOptionBarWindow->hWndFontCombo = hFontCombo;

	HWND hTextSizeCombo = CreateWindowEx(
		0,
		WC_COMBOBOX,
		L"",
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0,
		0,
		0,
		0,
		hWnd,
		(HMENU)(INT_PTR)IDCB_TEXT_SIZE,
		hInstance,
		NULL);
	Win32_ApplyUIFont(hTextSizeCombo);
	OptionBarWindow_PopulateTextSizeCombo(hTextSizeCombo);
	pOptionBarWindow->hWndTextSizeCombo = hTextSizeCombo;

	RECT rcClient = { 0 };
	GetClientRect(hWnd, &rcClient);
	OptionBarWindow_LayoutControls(
		pOptionBarWindow,
		rcClient.right - rcClient.left,
		rcClient.bottom - rcClient.top);

	OptionBarWindow_SyncTool(pOptionBarWindow, PanitentApp_GetTool(PanitentApp_Instance()));
	return TRUE;
}

void OptionBarWindow_OnSize(OptionBarWindow* pOptionBarWindow, UINT state, int cx, int cy)
{
	UNREFERENCED_PARAMETER(state);

	OptionBarWindow_LayoutControls(pOptionBarWindow, cx, cy);
}

LRESULT OptionBarWindow_UserProc(OptionBarWindow* pOptionBarWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
		return OptionBarWindow_OnCommand(pOptionBarWindow, wParam, lParam);
	case WM_SIZE:
		OptionBarWindow_OnSize(
			pOptionBarWindow,
			(UINT)wParam,
			(int)(short)LOWORD(lParam),
			(int)(short)HIWORD(lParam));
		return 0;
	case WM_OPTIONBAR_REFRESH_STATE:
		OptionBarWindow_SyncTool(pOptionBarWindow, PanitentApp_GetTool(PanitentApp_Instance()));
		return 0;
	}

	return Window_UserProcDefault((Window *)pOptionBarWindow, hWnd, message, wParam, lParam);
}

typedef struct _BrushDlgData {
	int iPvWidth;
	int iPvHeight;
	HBITMAP hPvBitmap;
	uint32_t* pPvBuffer;
} BrushDlgData;

void DrawBrushPreview(HWND hwnd)
{
	BrushDlgData* data = (BrushDlgData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	assert(data);
	if (!data)
		return;

	int width = data->iPvWidth;
	int height = data->iPvHeight;

	Canvas* canvas = Canvas_Create(width, height);
	if (!canvas)
		return;
	Canvas_FillSolid(canvas, 0xFFFFFFFF);

	Brush* brush = BrushBuilder_Build(g_pBrush, g_brushSize);
	if (!brush)
	{
		Canvas_Delete(canvas);
		return;
	}

	Brush_BezierCurve2(brush, canvas,
		32, height / 2,
		32 + (width - 64) / 4, height / 4,
		width - 32 - (width - 64) / 4, height / 4 * 3,
		width - 32, height / 2, 0xFF000000);

	for (int y = height; y--;)
	{
		memcpy(data->pPvBuffer + (size_t)y * (size_t)width, ((uint32_t*)canvas->buffer) + (size_t)y * (size_t)width,
			(size_t)width * 4);
	}

	Brush_Delete(brush);
	Canvas_Delete(canvas);
}

#define BRUSHSIZEMAX 128

INT_PTR CALLBACK BrushProp_DlgProc(HWND hwndDlg, UINT message, WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		BrushDlgData* data = malloc(sizeof(BrushDlgData));
		memset(data, 0, sizeof(BrushDlgData));
		if (!data)
			return TRUE;

		ZeroMemory(data, sizeof(BrushDlgData));
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)data);

		HWND hList = GetDlgItem(hwndDlg, IDC_BRUSHLIST);
		InitializeBrushList();

		/* Add brush items to list */
		int nItem;
		for (size_t i = 0; i < g_brushListLen; i++)
		{
			/* What 0 actually do? */
			nItem = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)NULL);
			SendMessage(hList, LB_SETITEMDATA, (WPARAM)nItem,
				(LPARAM)&g_brushList[i]);
		}
		for (size_t i = 0; i < g_brushListLen; ++i)
		{
			if (&g_brushList[i] == g_pBrush)
			{
				SendMessage(hList, LB_SETCURSEL, (WPARAM)i, 0);
				break;
			}
		}

		/* Create preview bitmap */
		HWND hPreview = GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW);

		RECT pvRc = { 0 };
		GetClientRect(hPreview, &pvRc);

		int width = pvRc.right;
		int height = pvRc.bottom;

		BITMAPINFO bmi = { 0 };
		Win32_InitGdiBitmapInfo32Bpp(&bmi, width, height);

		uint32_t* buffer = NULL;

		HDC hdc = GetDC(hPreview);
		HBITMAP hbmPv = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
			(LPVOID*)&buffer, NULL, 0);
		assert(hbmPv);
		assert(buffer);

		data->iPvWidth = width;
		data->iPvHeight = height;
		data->hPvBitmap = hbmPv;
		data->pPvBuffer = buffer;

		/*
		 *  Preview would be drawn here but freezes for an unknown reason
		 *  Perhaps GDI is cloning it internally if there any pixels in pszBuffer
		 *  I have read something like this on MSDN
		 */
		SendMessage(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), STM_SETIMAGE,
			(WPARAM)IMAGE_BITMAP, (LPARAM)hbmPv);

		DrawBrushPreview(hwndDlg);
		InvalidateRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), NULL, TRUE);

		/* Initialize size slider */
		HWND hTrack = GetDlgItem(hwndDlg, IDC_BRUSHSIZE);
		SendMessage(hTrack, TBM_SETRANGE, (WPARAM)TRUE,
			(LPARAM)MAKELONG(1, BRUSHSIZEMAX));
		SendMessage(hTrack, TBM_SETPAGESIZE, 0, (LPARAM)4);
		SendMessage(hTrack, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)g_brushSize);

	}
	return TRUE;
	case WM_HSCROLL:
		switch (LOWORD(wParam))
		{
		case TB_LINEUP:
		case TB_LINEDOWN:
		case TB_PAGEUP:
		case TB_PAGEDOWN:
		case TB_THUMBTRACK:
		{
				DWORD dwPos = (DWORD)SendMessage(GetDlgItem(hwndDlg, IDC_BRUSHSIZE),
					TBM_GETPOS, 0, 0);

				g_brushSize = max(1, (int)dwPos);

			DrawBrushPreview(hwndDlg);
			InvalidateRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), NULL, TRUE);
		}
		break;
		}
		return TRUE;

	case WM_MEASUREITEM:
	{
		PMEASUREITEMSTRUCT pmis = (PMEASUREITEMSTRUCT)lParam;

		pmis->itemHeight = 32;
	}
	return TRUE;

	case WM_DRAWITEM:
	{
		PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;

		if (pdis->itemID == (DWORD)-1)
			break;

		switch (pdis->itemAction)
		{
		case ODA_SELECT:
		case ODA_DRAWENTIRE:
		{
			FillRect(pdis->hDC, &pdis->rcItem,
				GetSysColorBrush(COLOR_WINDOW));

			if (pdis->itemState & ODS_SELECTED)
			{
				FillRect(pdis->hDC, &pdis->rcItem,
					GetSysColorBrush(COLOR_HIGHLIGHT));
			}

			BrushBuilder* brushBuilder = (BrushBuilder*)SendMessage(
				pdis->hwndItem, LB_GETITEMDATA, pdis->itemID, 0);

			Brush* brush = BrushBuilder_Build(brushBuilder, 16);
			assert(brush);

			int width = pdis->rcItem.right - pdis->rcItem.left;
			int height = pdis->rcItem.bottom - pdis->rcItem.top;

			uint32_t brColor = pdis->itemState & ODS_SELECTED ? 0xFFFFFFFF :
				0xFF000000;
			Canvas* canvas = Canvas_Create(width, height);
			Brush_Draw(brush, 16, 16, canvas, brColor);

			Brush_BezierCurve2(brush, canvas,
				48, 16,
				48 + 32, 8,
				width - 16 - 32, 24,
				width - 16, 16, brColor);

			/* Create GDI bitmap */
			BITMAPINFO bmi = { 0 };
			Win32_InitGdiBitmapInfo32Bpp(&bmi, canvas->width, canvas->height);

			uint32_t* buffer = NULL;

			HBITMAP hBitmap = CreateDIBSection(pdis->hDC, &bmi,
				DIB_RGB_COLORS, (LPVOID*)&buffer, NULL, 0);
			assert(hBitmap != NULL);
			assert(buffer != NULL);

			for (int y = height; y--;)
			{
				memcpy(buffer + (size_t)y * (size_t)width, ((uint32_t*)canvas->buffer) + (size_t)y *
					(size_t)width, (size_t)width * 4);
			}

			HDC hdcMem = CreateCompatibleDC(pdis->hDC);
			HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);

			BLENDFUNCTION blendFunc = {
			  .BlendOp = AC_SRC_OVER,
			  .BlendFlags = 0,
			  .SourceConstantAlpha = 0xFF,
			  .AlphaFormat = AC_SRC_ALPHA
			};

			AlphaBlend(
				pdis->hDC,
				pdis->rcItem.left,
				pdis->rcItem.top,
				width,
				height,
				hdcMem,
				0, 0,
				width,
				height,
				blendFunc);

			/*
			BitBlt(
				pdis->hDC,
				pdis->rcItem.left,
				pdis->rcItem.top,
				pdis->rcItem.right - pdis->rcItem.left,
				pdis->rcItem.bottom - pdis->rcItem.top,
				hdcMem, 0, 0, SRCCOPY);
				*/

			SelectObject(hdcMem, hOldObj);

			DeleteObject(hBitmap);
			DeleteDC(hdcMem);

			Canvas_Delete(canvas);
		}
		break;

		case ODA_FOCUS:
			DrawFocusRect(pdis->hDC, &pdis->rcItem);
			break;
		}
	}
	return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BRUSHLIST && HIWORD(wParam) == LBN_SELCHANGE)
		{
			HWND hList = GetDlgItem(hwndDlg, IDC_BRUSHLIST);

			int nItem = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
			BrushBuilder* builder = (BrushBuilder*)SendMessage(hList,
				LB_GETITEMDATA, nItem, 0);

			g_pBrush = builder;

			DrawBrushPreview(hwndDlg);
			InvalidateRect(GetDlgItem(hwndDlg, IDC_BRUSHPREVIEW), NULL, TRUE);
			InvalidateRect(GetParent(hwndDlg), NULL, TRUE);
		}

		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDCANCEL:
				/* fall through */
			case IDOK:
				EndDialog(hwndDlg, wParam);
				break;
			}
		}
		return TRUE;
	}

	return FALSE;
}

void OptionBarWindow_OnDestroy(OptionBarWindow* pOptionBarWindow)
{
}

void OptionBarWindow_OnPaint(OptionBarWindow* pOptionBarWindow)
{
	HWND hWnd = Window_GetHWND((Window *)pOptionBarWindow);

	PAINTSTRUCT ps = { 0 };
	HDC hdc = BeginPaint(hWnd, &ps);

	EndPaint(hWnd, &ps);
}

void OptionBarWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
	lpwcex->style = CS_HREDRAW | CS_VREDRAW;
	lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
	lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	lpwcex->lpszClassName = szClassName;
}

void OptionBarWindow_PreCreate(LPCREATESTRUCT lpcs)
{
	lpcs->dwExStyle = 0;
	lpcs->lpszClass = szClassName;
	lpcs->lpszName = L"Option Bar";
	lpcs->style = WS_CAPTION | WS_OVERLAPPED | WS_VISIBLE;
	lpcs->x = CW_USEDEFAULT;
	lpcs->y = CW_USEDEFAULT;
	lpcs->cx = 300;
	lpcs->cy = 200;
}

void OptionBarWindow_Init(OptionBarWindow* pOptionBarWindow)
{
	Window_Init(&pOptionBarWindow->base);

	pOptionBarWindow->base.szClassName = szClassName;

	pOptionBarWindow->base.OnCreate = (FnWindowOnCreate)OptionBarWindow_OnCreate;
	pOptionBarWindow->base.OnDestroy = (FnWindowOnDestroy)OptionBarWindow_OnDestroy;
	pOptionBarWindow->base.OnPaint = (FnWindowOnPaint)OptionBarWindow_OnPaint;
	pOptionBarWindow->base.OnSize = (FnWindowOnSize)OptionBarWindow_OnSize;
	
	_WindowInitHelper_SetPreRegisterRoutine((Window *)pOptionBarWindow, (FnWindowPreRegister)OptionBarWindow_PreRegister);
	_WindowInitHelper_SetPreCreateRoutine((Window *)pOptionBarWindow, (FnWindowPreCreate)OptionBarWindow_PreCreate);
	_WindowInitHelper_SetUserProcRoutine((Window *)pOptionBarWindow, (FnWindowUserProc)OptionBarWindow_UserProc);
}

OptionBarWindow* OptionBarWindow_Create()
{
	OptionBarWindow* pOptionBarWindow = (OptionBarWindow*)malloc(sizeof(OptionBarWindow));
	if (pOptionBarWindow)
	{
		memset(pOptionBarWindow, 0, sizeof(OptionBarWindow));
		OptionBarWindow_Init(pOptionBarWindow);
	}

	return pOptionBarWindow;
}


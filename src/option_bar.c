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

static const WCHAR szClassName[] = L"__OptionBarWindow";

#define IDCB_STENCIL_ALGORITHM 1553
#define IDCB_THICKNESS 1554
#define IDB_SHAPESTROKE 1555
#define IDB_SHAPEFILL 1556
#define IDC_OPTIONBAR_TEXT_EDIT 2002

static const int kOptionBarPadding = 8;
static const int kOptionBarControlTop = 4;
static const int kOptionBarControlHeight = 22;
static const int kOptionBarStrokeWidth = 66;
static const int kOptionBarFillWidth = 54;
static const int kOptionBarAlgorithmWidth = 112;
static const int kOptionBarThicknessWidth = 72;
static const int kOptionBarTextWidth = 150;
static const int kOptionBarBrushWidth = 64;
static const int kOptionBarControlGap = 6;

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

	HWND hWndStrokeCheck = pOptionBarWindow->hWndStrokeCheck;
	HWND hWndFillCheck = pOptionBarWindow->hWndFillCheck;
	HWND hWndAlgorithmCombo = pOptionBarWindow->hWndAlgorithmCombo;
	HWND hWndThicknessCombo = pOptionBarWindow->hWndThicknessCombo;
	HWND hWndTextEdit = pOptionBarWindow->textstring_handle;
	HWND hWndBrushSelector = pOptionBarWindow->hWndBrushSelector;
	if (!hWndStrokeCheck || !hWndFillCheck || !hWndAlgorithmCombo || !hWndThicknessCombo || !hWndTextEdit || !hWndBrushSelector)
	{
		return;
	}

	int y = kOptionBarControlTop;
	int x = kOptionBarPadding;
	int strokeWidth = kOptionBarStrokeWidth;
	int fillWidth = kOptionBarFillWidth;
	int algorithmWidth = kOptionBarAlgorithmWidth;
	int thicknessWidth = kOptionBarThicknessWidth;
	int brushWidth = kOptionBarBrushWidth;
	int textWidth = kOptionBarTextWidth;

	int leftGroupEnd = x + strokeWidth + kOptionBarControlGap +
		fillWidth + kOptionBarControlGap +
		algorithmWidth + kOptionBarControlGap +
		thicknessWidth;

	int brushX = cx - kOptionBarPadding - brushWidth;
	int editX = brushX - kOptionBarControlGap - textWidth;
	if (editX < leftGroupEnd + kOptionBarControlGap)
	{
		editX = leftGroupEnd + kOptionBarControlGap;
		textWidth = brushX - kOptionBarControlGap - editX;
		if (textWidth < 80)
		{
			textWidth = 80;
			brushX = editX + textWidth + kOptionBarControlGap;
		}
	}

	int currentX = x;
	MoveWindow(hWndStrokeCheck, currentX, y + 1, strokeWidth, kOptionBarControlHeight, TRUE);
	currentX += strokeWidth + kOptionBarControlGap;
	MoveWindow(hWndFillCheck, currentX, y + 1, fillWidth, kOptionBarControlHeight, TRUE);
	currentX += fillWidth + kOptionBarControlGap;
	MoveWindow(hWndAlgorithmCombo, currentX, y, algorithmWidth, kOptionBarControlHeight + 200, TRUE);
	currentX += algorithmWidth + kOptionBarControlGap;
	MoveWindow(hWndThicknessCombo, currentX, y, thicknessWidth, kOptionBarControlHeight + 200, TRUE);

	MoveWindow(hWndTextEdit, editX, y, textWidth, kOptionBarControlHeight, TRUE);
	MoveWindow(hWndBrushSelector, brushX, y, brushWidth, kOptionBarControlHeight, TRUE);
}

LRESULT OptionBarWindow_OnCommand(OptionBarWindow* pOptionBarWindow, WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(pOptionBarWindow);
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);
	/*
	primitives_context_t* primitivesContext = PanitentApp_GetPrimitivesContext(PanitentApp_Instance());

	if (HIWORD(wparam) == BN_CLICKED)
	{
		switch (LOWORD(wparam))
		{
		case IDB_SHAPESTROKE:
			primitivesContext->fStroke = Button_GetCheck((HWND)lparam);
			break;
		case IDB_SHAPEFILL:
			primitivesContext->fFill = Button_GetCheck((HWND)lparam);
			break;
		}
	}

	if (HIWORD(wparam) != LBN_SELCHANGE)
	{
		return 0;
	}

	switch (LOWORD(wparam))
	{
	case IDCB_STENCIL_ALGORITHM:
		switch (ComboBox_GetCurSel((HWND)lparam))
		{
		case 0:
			PanitentApp_SetPrimitivesContext(PanitentApp_Instance(), &g_bresenham_primitives);
			break;
		case 1:
			PanitentApp_SetPrimitivesContext(PanitentApp_Instance(), &g_wu_primitives);
			break;
		}
		break;
	case IDCB_THICKNESS:
		SetThickness(ComboBox_GetCurSel((HWND)lparam) + 1);
		break;
	}

	if (LOWORD(wparam) == IDCB_STENCIL_ALGORITHM &&
		HIWORD(wparam) == LBN_SELCHANGE) {
		switch (ComboBox_GetCurSel((HWND)lparam)) {
			break;
		default:
			break;
		}
	}
	*/

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
	ComboBox_SetCurSel(hAlgorithmCombo, 0);
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
	for (int i = 1; i <= 10; ++i)
	{
		WCHAR szThickness[4] = L"";
		_itow_s(i, szThickness, ARRAYSIZE(szThickness), 10);
		ComboBox_AddString(hThicknessCombo, szThickness);
	}
	ComboBox_SetCurSel(hThicknessCombo, 0);
	pOptionBarWindow->hWndThicknessCombo = hThicknessCombo;

	HWND hEdit =
		CreateWindowEx(0,
			WC_EDIT,
			L"Sample Text",
			WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			0,
			0,
			0,
			0,
			hWnd,
			(HMENU)(INT_PTR)IDC_OPTIONBAR_TEXT_EDIT,
			hInstance,
			NULL);
	Win32_ApplyUIFont(hEdit);
	pOptionBarWindow->textstring_handle = hEdit;

	HWND hBrushSelector = CreateWindowEx(0, szBrushSelClass, NULL,
		WS_BORDER | WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		hWnd, NULL, hInstance, NULL);
	pOptionBarWindow->hWndBrushSelector = hBrushSelector;

	RECT rcClient = { 0 };
	GetClientRect(hWnd, &rcClient);
	OptionBarWindow_LayoutControls(
		pOptionBarWindow,
		rcClient.right - rcClient.left,
		rcClient.bottom - rcClient.top);

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
		break;
	case WM_SIZE:
		OptionBarWindow_OnSize(
			pOptionBarWindow,
			(UINT)wParam,
			(int)(short)LOWORD(lParam),
			(int)(short)HIWORD(lParam));
		return 0;
		break;
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


#include "common.h"

#include "util.h"

HFONT g_hUIFont;

void Win32_FetchUIFont()
{
	NONCLIENTMETRICS ncm = { 0 };
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

	BOOL bResult = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	if (!bResult)
	{
		return NULL;
	}

	HFONT hNewFont = CreateFontIndirect(&ncm.lfMessageFont);
	if (!hNewFont)
	{
		return NULL;
	}

	DeleteObject(g_hUIFont);
	g_hUIFont = hNewFont;
}

HFONT Win32_GetUIFont()
{
	if (!g_hUIFont)
	{
		Win32_FetchUIFont();
	}

	return g_hUIFont;
}

void Win32_ApplyUIFont(HWND hWnd)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)g_hUIFont, MAKELPARAM(FALSE, 0));
}

/*
 *  For API calls that doesn't require biSizeImage to be precalculated
 *  In special cases use this:
 *
 *  bmi.bmiHeader.biSizeImage = ((bmi.bmiHeader.biWidth *
 *      bmi.bmiHeader.biBitCount + 31) / 32) * 4 * bmi.bmiHeader.biHeight;
 */
void Win32_InitGdiBitmapInfo32Bpp(LPBITMAPINFO bmi, LONG width, LONG height)
{
	ZeroMemory(bmi, sizeof(BITMAPINFO));

	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = width;
	bmi->bmiHeader.biHeight = height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 32;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;
}

void Win32_ContractRect(LPRECT rc, int x, int y)
{
	rc->left += x;
	rc->top += y;
	rc->right -= x;
	rc->bottom -= y;
}

void swapf(float* a, float* b)
{
	float c = *a;
	*b = *a;
	*a = c;
}

COLORREF Win32_HexToCOLORREF(LPWSTR pszHexColor)
{
	if (pszHexColor[0] == L'#')
	{
		pszHexColor++;
	}

	// Conver the gxadecmal string to an integer
	DWORD dwHexValue = wcstoul(pszHexColor, NULL, 16);

	// Extract the red, green, and blue components from the integer value
	BYTE red = (dwHexValue >> 16) & 0xFF;
	BYTE green = (dwHexValue >> 8) & 0xFF;
	BYTE blue = dwHexValue & 0xFF;

	// Create and return the COLORREF value
	return RGB(red, green, blue);
}

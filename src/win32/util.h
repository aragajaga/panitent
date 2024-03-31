#pragma once

void Win32_FetchUIFont();
HFONT Win32_GetUIFont();
void Win32_ApplyUIFont(HWND hWnd);
void Win32_InitGdiBitmapInfo32Bpp(LPBITMAPINFO bmi, LONG width, LONG height);

void Win32_ContractRect(LPRECT rc, int x, int y);
COLORREF Win32_HexToCOLORREF(LPWSTR pszHexValue);

void swapf(float* a, float* b);

#define COLORREF_WHITE RGB(0xFF, 0xFF, 0xFF)
#define COLORREF_ORANGE RGB(0xFF, 0x88, 0x00)
#define COLORREF_RED RGB(0xFF, 0x00, 0x00)

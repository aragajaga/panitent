#pragma once

#define PtIn(pt, left, top, right, bottom) ((pt).x >= (left) && (pt).y >= (top) && (pt).x < (right) && (pt).y < (bottom))

#define RECTWIDTH(prc) ((prc)->right - (prc)->left)
#define RECTHEIGHT(prc) ((prc)->bottom - (prc)->top)

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
#define COLORREF_MAGENTA RGB(0xFF, 0x00, 0xFF)

typedef struct Window Window;

void Win32_FitChild(Window* pChildWindow, Window* pParentWindow);

ULONG64 Win32_GetSystemTimeAsUnixTime();

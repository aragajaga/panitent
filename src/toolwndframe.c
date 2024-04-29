#include "precomp.h"

#include "toolwndframe.h"

#include "win32/util.h"
#include "resource.h"

#define GLYPH_SIZE 8

void DrawCaptionGlyph(HDC hdc, int x, int y, int iGlyph)
{
    HBITMAP hbmGlyphs = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FLOATINGGLYPHS));
    HDC hdcGlyphs = CreateCompatibleDC(hdc);
    HBITMAP hPrevBm = SelectObject(hdcGlyphs, hbmGlyphs);

    TransparentBlt(hdc, x, y, GLYPH_SIZE, GLYPH_SIZE, hdcGlyphs, iGlyph * GLYPH_SIZE, 0, GLYPH_SIZE, GLYPH_SIZE, COLORREF_MAGENTA);

    SelectObject(hdcGlyphs, hPrevBm);
    DeleteObject(hbmGlyphs);
}

void DrawCaptionButton(CaptionButton* pCaptionButton, HDC hdc, int x, int y)
{
    RECT rcButton = { 0 };
    rcButton.left = x;
    rcButton.top = y;
    rcButton.right = x + pCaptionButton->size.cx;
    rcButton.bottom = y + pCaptionButton->size.cy;

    SetDCPenColor(hdc, RGB(0xFF, 0xFF, 0xFF));
    SetDCBrushColor(hdc, Win32_HexToCOLORREF(L"#9185be"));
    SelectObject(hdc, GetStockObject(DC_PEN));
    SelectObject(hdc, GetStockObject(DC_BRUSH));

    // Rectangle(hdc, rcButton.left, rcButton.top, rcButton.right, rcButton.bottom);

    DrawCaptionGlyph(hdc, rcButton.left + (rcButton.right - rcButton.left - 8) / 2, rcButton.top + (rcButton.bottom - rcButton.top - 8) / 2, pCaptionButton->glyph);
}

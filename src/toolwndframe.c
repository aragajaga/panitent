#include "precomp.h"

#include "toolwndframe.h"

#include "win32/util.h"
#include "resource.h"
#include "util/assert.h"

#define GLYPH_SIZE 8

void DrawCaptionGlyph(HDC hdc, PRECT prc, int iGlyph)
{
    HBITMAP hbmGlyphs = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FLOATINGGLYPHS));
    HDC hdcGlyphs = CreateCompatibleDC(hdc);

    BITMAP bm;
    ASSERT(GetObject(hbmGlyphs, sizeof(BITMAP), &bm));

    int glyphSize = bm.bmHeight;

    HBITMAP hOldBm = SelectObject(hdcGlyphs, hbmGlyphs);

    // Rectangle(hdc, x, y, x + glyphSize, y + glyphSize);

    int x = prc->left + ((prc->right - prc->left) - glyphSize) / 2;
    int y = prc->top + ((prc->bottom - prc->top) - glyphSize) / 2;
    TransparentBlt(hdc, x, y, glyphSize, glyphSize, hdcGlyphs, iGlyph * glyphSize, 0, glyphSize, glyphSize, COLORREF_MAGENTA);

    // Select previous object
    SelectObject(hdcGlyphs, hOldBm);

    // Free bitmap object
    DeleteObject(hbmGlyphs);
}

void DrawCaptionButton(CaptionButton* pCaptionButton, HDC hdc, int x, int y, int width, int height)
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

    Rectangle(hdc, rcButton.left, rcButton.top, rcButton.right, rcButton.bottom);

    RECT rc = { 0 };
    rc.left = x;
    rc.top = y;
    rc.right = x + width;
    rc.bottom = y + height;
    DrawCaptionGlyph(hdc, &rc, pCaptionButton->glyph);
}

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

BOOL CaptionFrame_BuildLayout(const RECT* pRectFrame, const CaptionFrameMetrics* pMetrics, const CaptionButton* pButtons, int nButtons, CaptionFrameLayout* pLayout)
{
    if (!pRectFrame || !pMetrics || !pLayout)
    {
        return FALSE;
    }

    memset(pLayout, 0, sizeof(*pLayout));
    pLayout->rcFrame = *pRectFrame;
    SetRectEmpty(&pLayout->rcCaption);
    SetRectEmpty(&pLayout->rcCaptionText);

    RECT rcCaption = *pRectFrame;
    rcCaption.left += pMetrics->borderSize;
    rcCaption.top += pMetrics->borderSize;
    rcCaption.right -= pMetrics->borderSize;
    rcCaption.bottom = min(rcCaption.bottom - pMetrics->borderSize, rcCaption.top + pMetrics->captionHeight);
    if (rcCaption.right <= rcCaption.left || rcCaption.bottom <= rcCaption.top)
    {
        return FALSE;
    }

    pLayout->rcCaption = rcCaption;
    pLayout->nButtons = 0;

    int clampedButtons = max(0, min(nButtons, CAPTION_FRAME_MAX_BUTTONS));
    int xRight = rcCaption.right;
    for (int i = 0; i < clampedButtons; ++i)
    {
        const CaptionButton* pButton = &pButtons[i];
        int buttonWidth = (pButton->size.cx > 0) ? pButton->size.cx : pMetrics->captionHeight;
        int buttonHeight = (pButton->size.cy > 0) ? pButton->size.cy : pMetrics->captionHeight;
        if (buttonWidth <= 0 || buttonHeight <= 0)
        {
            continue;
        }

        RECT rcButton = { 0 };
        rcButton.right = xRight;
        rcButton.left = rcButton.right - buttonWidth;
        rcButton.top = rcCaption.top;
        rcButton.bottom = rcButton.top + buttonHeight;
        if (rcButton.left < rcCaption.left)
        {
            break;
        }

        int index = pLayout->nButtons++;
        pLayout->buttons[index] = *pButton;
        pLayout->buttonRects[index] = rcButton;

        xRight = rcButton.left - pMetrics->buttonSpacing;
    }

    RECT rcText = rcCaption;
    rcText.left += pMetrics->textPaddingX;
    rcText.top += pMetrics->textPaddingY;
    rcText.bottom -= pMetrics->textPaddingY;
    if (pLayout->nButtons > 0)
    {
        rcText.right = min(rcText.right, pLayout->buttonRects[pLayout->nButtons - 1].left - pMetrics->textPaddingX);
    }
    else {
        rcText.right -= pMetrics->textPaddingX;
    }

    if (rcText.right < rcText.left)
    {
        rcText.right = rcText.left;
    }
    if (rcText.bottom < rcText.top)
    {
        rcText.bottom = rcText.top;
    }
    pLayout->rcCaptionText = rcText;

    return TRUE;
}

int CaptionFrame_HitTestButton(const CaptionFrameLayout* pLayout, POINT pt)
{
    if (!pLayout)
    {
        return HTNOWHERE;
    }

    for (int i = 0; i < pLayout->nButtons; ++i)
    {
        if (PtInRect(&pLayout->buttonRects[i], pt))
        {
            return pLayout->buttons[i].htCommand;
        }
    }

    return HTNOWHERE;
}

BOOL CaptionFrame_GetButtonRect(const CaptionFrameLayout* pLayout, int htCommand, RECT* pRect)
{
    if (!pLayout || !pRect)
    {
        return FALSE;
    }

    for (int i = 0; i < pLayout->nButtons; ++i)
    {
        if (pLayout->buttons[i].htCommand == htCommand)
        {
            *pRect = pLayout->buttonRects[i];
            return TRUE;
        }
    }

    return FALSE;
}

void CaptionFrame_Draw(HDC hdc, const CaptionFrameLayout* pLayout, const CaptionFramePalette* pPalette, PCWSTR pszCaption, HFONT hFont)
{
    if (!hdc || !pLayout || !pPalette)
    {
        return;
    }

    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));
    SetDCBrushColor(hdc, pPalette->frameFill);
    SetDCPenColor(hdc, pPalette->border);
    Rectangle(hdc, pLayout->rcFrame.left, pLayout->rcFrame.top, pLayout->rcFrame.right, pLayout->rcFrame.bottom);

    if (!IsRectEmpty(&pLayout->rcCaption))
    {
        SetDCBrushColor(hdc, pPalette->captionFill);
        FillRect(hdc, &pLayout->rcCaption, (HBRUSH)GetStockObject(DC_BRUSH));
    }

    if (pszCaption && pszCaption[0] && pLayout->rcCaptionText.right > pLayout->rcCaptionText.left)
    {
        HFONT hOldFont = NULL;
        if (hFont)
        {
            hOldFont = (HFONT)SelectObject(hdc, hFont);
        }

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, pPalette->text);
        DrawText(hdc, pszCaption, -1, (LPRECT)&pLayout->rcCaptionText, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        if (hOldFont)
        {
            SelectObject(hdc, hOldFont);
        }
    }

    for (int i = 0; i < pLayout->nButtons; ++i)
    {
        RECT rc = pLayout->buttonRects[i];
        CaptionButton button = pLayout->buttons[i];
        DrawCaptionButton(&button, hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
}

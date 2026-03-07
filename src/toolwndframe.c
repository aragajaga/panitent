#include "precomp.h"

#include "toolwndframe.h"

#include "win32/util.h"
#include "resource.h"
#include "util/assert.h"

#define GLYPH_SIZE 8

static BYTE CaptionFrame_ClampColorChannel(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 255)
    {
        return 255;
    }
    return (BYTE)value;
}

static COLORREF CaptionFrame_AdjustColor(COLORREF color, int delta)
{
    int r = (int)GetRValue(color) + delta;
    int g = (int)GetGValue(color) + delta;
    int b = (int)GetBValue(color) + delta;
    return RGB(
        CaptionFrame_ClampColorChannel(r),
        CaptionFrame_ClampColorChannel(g),
        CaptionFrame_ClampColorChannel(b));
}

void DrawCaptionGlyph(HDC hdc, PRECT prc, int iGlyph)
{
    if (!hdc || !prc)
    {
        return;
    }

    HBITMAP hbmGlyphs = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FLOATINGGLYPHS));
    if (!hbmGlyphs)
    {
        return;
    }

    HDC hdcGlyphs = CreateCompatibleDC(hdc);
    if (!hdcGlyphs)
    {
        DeleteObject(hbmGlyphs);
        return;
    }

    BITMAP bm = { 0 };
    if (!GetObject(hbmGlyphs, sizeof(BITMAP), &bm))
    {
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    int glyphSize = bm.bmHeight;
    if (glyphSize <= 0)
    {
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    int glyphCount = bm.bmWidth / glyphSize;
    if (glyphCount <= 0)
    {
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    if (iGlyph < 0)
    {
        iGlyph = 0;
    }
    if (iGlyph >= glyphCount)
    {
        iGlyph = glyphCount - 1;
    }

    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcGlyphs, hbmGlyphs);
    if (!hOldBm)
    {
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = glyphSize;
    bmi.bmiHeader.biHeight = -glyphSize; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t* pGlyphPixels = NULL;
    HBITMAP hbmColoredGlyph = CreateDIBSection(
        hdc,
        &bmi,
        DIB_RGB_COLORS,
        (LPVOID*)&pGlyphPixels,
        NULL,
        0);
    if (!hbmColoredGlyph || !pGlyphPixels)
    {
        SelectObject(hdcGlyphs, hOldBm);
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    HDC hdcColoredGlyph = CreateCompatibleDC(hdc);
    if (!hdcColoredGlyph)
    {
        DeleteObject(hbmColoredGlyph);
        SelectObject(hdcGlyphs, hOldBm);
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    HBITMAP hOldColored = (HBITMAP)SelectObject(hdcColoredGlyph, hbmColoredGlyph);
    if (!hOldColored)
    {
        DeleteDC(hdcColoredGlyph);
        DeleteObject(hbmColoredGlyph);
        SelectObject(hdcGlyphs, hOldBm);
        DeleteDC(hdcGlyphs);
        DeleteObject(hbmGlyphs);
        return;
    }

    const COLORREF glyphColor = RGB(255, 255, 255);
    const BYTE targetR = GetRValue(glyphColor);
    const BYTE targetG = GetGValue(glyphColor);
    const BYTE targetB = GetBValue(glyphColor);

    int srcX = iGlyph * glyphSize;
    for (int y = 0; y < glyphSize; ++y)
    {
        for (int x = 0; x < glyphSize; ++x)
        {
            COLORREF srcColor = GetPixel(hdcGlyphs, srcX + x, y);
            BYTE mask = (BYTE)(((int)GetRValue(srcColor) + (int)GetGValue(srcColor) + (int)GetBValue(srcColor)) / 3);

            /* AlphaBlend expects premultiplied RGB for AC_SRC_ALPHA. */
            BYTE outR = (BYTE)(((int)targetR * (int)mask + 127) / 255);
            BYTE outG = (BYTE)(((int)targetG * (int)mask + 127) / 255);
            BYTE outB = (BYTE)(((int)targetB * (int)mask + 127) / 255);

            pGlyphPixels[(size_t)y * (size_t)glyphSize + (size_t)x] =
                ((uint32_t)mask << 24) |
                ((uint32_t)outR << 16) |
                ((uint32_t)outG << 8) |
                (uint32_t)outB;
        }
    }

    int x = prc->left + ((prc->right - prc->left) - glyphSize) / 2;
    int y = prc->top + ((prc->bottom - prc->top) - glyphSize) / 2;
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 0xFF;
    blend.AlphaFormat = AC_SRC_ALPHA;
    AlphaBlend(hdc, x, y, glyphSize, glyphSize, hdcColoredGlyph, 0, 0, glyphSize, glyphSize, blend);

    SelectObject(hdcColoredGlyph, hOldColored);
    DeleteDC(hdcColoredGlyph);
    DeleteObject(hbmColoredGlyph);
    SelectObject(hdcGlyphs, hOldBm);
    DeleteDC(hdcGlyphs);
    DeleteObject(hbmGlyphs);
}

void DrawCaptionButton(CaptionButton* pCaptionButton, HDC hdc, int x, int y, int width, int height, COLORREF fill)
{
    RECT rcButton = { 0 };
    rcButton.left = x;
    rcButton.top = y;
    rcButton.right = x + pCaptionButton->size.cx;
    rcButton.bottom = y + pCaptionButton->size.cy;

    SetDCBrushColor(hdc, fill);
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    FillRect(hdc, &rcButton, (HBRUSH)GetStockObject(DC_BRUSH));

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
    rcText.left += pMetrics->textPaddingLeft;
    rcText.top += pMetrics->textPaddingY;
    rcText.bottom -= pMetrics->textPaddingY;
    if (pLayout->nButtons > 0)
    {
        rcText.right = min(rcText.right, pLayout->buttonRects[pLayout->nButtons - 1].left - pMetrics->textPaddingRight);
    }
    else {
        rcText.right -= pMetrics->textPaddingRight;
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

void CaptionFrame_DrawStateful(
    HDC hdc,
    const CaptionFrameLayout* pLayout,
    const CaptionFramePalette* pPalette,
    PCWSTR pszCaption,
    HFONT hFont,
    int nHotButton,
    int nPressedButton)
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
        COLORREF buttonFill = pPalette->captionFill;
        if (button.htCommand == nPressedButton)
        {
            buttonFill = CaptionFrame_AdjustColor(pPalette->captionFill, -18);
        }
        else if (button.htCommand == nHotButton) {
            buttonFill = CaptionFrame_AdjustColor(pPalette->captionFill, 14);
        }

        DrawCaptionButton(&button, hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, buttonFill);
    }
}

void CaptionFrame_Draw(HDC hdc, const CaptionFrameLayout* pLayout, const CaptionFramePalette* pPalette, PCWSTR pszCaption, HFONT hFont)
{
    CaptionFrame_DrawStateful(hdc, pLayout, pPalette, pszCaption, hFont, HTNOWHERE, HTNOWHERE);
}

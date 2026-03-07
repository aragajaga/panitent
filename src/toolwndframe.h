#pragma once

enum {
    CAPTION_GLYPH_CHEVRON_TILE = 0,
    CAPTION_GLYPH_PIN_DIAGONAL_TILE = 1,
    CAPTION_GLYPH_MINIMIZE_TILE = 2,
    CAPTION_GLYPH_MAXIMIZE_TILE = 3,
    CAPTION_GLYPH_CLOSE_TILE = 4,
    CAPTION_GLYPH_RESTORE_TILE = 5,
    CAPTION_GLYPH_PIN_VERTICAL_TILE = 6
};

enum {
    GLYPH_MORE = CAPTION_GLYPH_CHEVRON_TILE,
    GLYPH_PIN = CAPTION_GLYPH_PIN_DIAGONAL_TILE,
    GLYPH_MINIMIZE = CAPTION_GLYPH_MINIMIZE_TILE,
    GLYPH_MAXIMIZE = CAPTION_GLYPH_MAXIMIZE_TILE,
    GLYPH_CLOSE = CAPTION_GLYPH_CLOSE_TILE,
    GLYPH_RESTORE = CAPTION_GLYPH_RESTORE_TILE,
    GLYPH_HELP = CAPTION_GLYPH_PIN_VERTICAL_TILE
};

typedef struct CaptionButton CaptionButton;

struct CaptionButton {
    SIZE size;
    int glyph;
    int htCommand;
};

typedef struct CaptionFrameMetrics CaptionFrameMetrics;
struct CaptionFrameMetrics {
    int borderSize;
    int captionHeight;
    int buttonSpacing;
    int textPaddingLeft;
    int textPaddingRight;
    int textPaddingY;
};

typedef struct CaptionFramePalette CaptionFramePalette;
struct CaptionFramePalette {
    COLORREF frameFill;
    COLORREF border;
    COLORREF captionFill;
    COLORREF text;
};

#define CAPTION_FRAME_MAX_BUTTONS 4

typedef struct CaptionFrameLayout CaptionFrameLayout;
struct CaptionFrameLayout {
    RECT rcFrame;
    RECT rcCaption;
    RECT rcCaptionText;
    CaptionButton buttons[CAPTION_FRAME_MAX_BUTTONS];
    RECT buttonRects[CAPTION_FRAME_MAX_BUTTONS];
    int nButtons;
};

void DrawCaptionGlyph(HDC hdc, PRECT prc, int iGlyph);
void DrawCaptionButton(CaptionButton* pCaptionButton, HDC hdc, int x, int y, int width, int height, COLORREF fill);
BOOL CaptionFrame_BuildLayout(const RECT* pRectFrame, const CaptionFrameMetrics* pMetrics, const CaptionButton* pButtons, int nButtons, CaptionFrameLayout* pLayout);
int CaptionFrame_HitTestButton(const CaptionFrameLayout* pLayout, POINT pt);
BOOL CaptionFrame_GetButtonRect(const CaptionFrameLayout* pLayout, int htCommand, RECT* pRect);
void CaptionFrame_Draw(HDC hdc, const CaptionFrameLayout* pLayout, const CaptionFramePalette* pPalette, PCWSTR pszCaption, HFONT hFont);
void CaptionFrame_DrawStateful(HDC hdc, const CaptionFrameLayout* pLayout, const CaptionFramePalette* pPalette, PCWSTR pszCaption, HFONT hFont, int nHotButton, int nPressedButton);

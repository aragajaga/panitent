#pragma once

enum {
    GLYPH_MORE = 0,
    GLYPH_PIN,
    GLYPH_MINIMIZE,
    GLYPH_MAXIMIZE,
    GLYPH_CLOSE,
    GLYPH_RESTORE,
    GLYPH_HELP
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
    int textPaddingX;
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

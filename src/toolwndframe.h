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
};

void DrawCaptionGlyph(HDC hdc, int x, int y, int iGlyph);
void DrawCaptionButton(CaptionButton* pCaptionButton, HDC hdc, int x, int y);

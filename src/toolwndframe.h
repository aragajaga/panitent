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

void DrawCaptionGlyph(HDC hdc, PRECT prc, int iGlyph);
void DrawCaptionButton(CaptionButton* pCaptionButton, HDC hdc, int x, int y, int width, int height);

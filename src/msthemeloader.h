#pragma once

typedef struct MSTheme MSTheme;
struct MSTheme {
    HMODULE hModule;
};

MSTheme* MSTheme_Create(PTSTR pszPath);
void MSTheme_Destroy(MSTheme* pMSTheme);

void MSTheme_DrawCloseBtn(MSTheme* pMSTheme, HDC hDC, int x, int y);
void MSTheme_DrawCloseGlyph(MSTheme* pMSTheme, HDC hDC, int x, int y);
void MSTheme_DrawAnything(MSTheme* pMSTheme, HDC hDC, PTSTR pszResourceName, int x, int y);

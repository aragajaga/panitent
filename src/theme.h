#ifndef PANITENT_THEME_H_
#define PANITENT_THEME_H_

#include "precomp.h"

#include "toolwndframe.h"

typedef struct PanitentThemeColors PanitentThemeColors;
struct PanitentThemeColors {
    COLORREF accent;
    COLORREF accentActive;
    COLORREF border;
    COLORREF rootBackground;
    COLORREF workspaceHeader;
    COLORREF menuBackground;
    COLORREF menuHot;
    COLORREF menuOpen;
    COLORREF splitterFill;
    COLORREF splitterGrip;
    COLORREF guideBackplate;
    COLORREF guideBackplateBorder;
    COLORREF text;
};

void PanitentTheme_GetDefaultHsl(int* pHue, int* pSaturation, int* pLightness);
void PanitentTheme_GetColors(PanitentThemeColors* pColors);
void PanitentTheme_GetColorsForHsl(int hue, int saturation, int lightness, PanitentThemeColors* pColors);
void PanitentTheme_GetCaptionPalette(CaptionFramePalette* pPalette);
void PanitentTheme_RefreshApplication(void);

#endif /* PANITENT_THEME_H_ */

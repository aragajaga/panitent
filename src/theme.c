#include "precomp.h"

#include "theme.h"

#include "dockhost.h"
#include "panitentapp.h"
#include "panitentwindow.h"

static int PanitentTheme_ClampInt(int value, int minimum, int maximum);
static COLORREF PanitentTheme_HslToColor(int hue, int saturation, int lightness);
static float PanitentTheme_HueToRgb(float p, float q, float t);
static BOOL CALLBACK PanitentTheme_EnumThreadWindowsProc(HWND hWnd, LPARAM lParam);

void PanitentTheme_GetDefaultHsl(int* pHue, int* pSaturation, int* pLightness)
{
    if (pHue)
    {
        *pHue = 253;
    }
    if (pSaturation)
    {
        *pSaturation = 30;
    }
    if (pLightness)
    {
        *pLightness = 63;
    }
}

void PanitentTheme_GetColors(PanitentThemeColors* pColors)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    int hue = 0;
    int saturation = 0;
    int lightness = 0;

    PanitentTheme_GetDefaultHsl(&hue, &saturation, &lightness);
    if (pPanitentApp)
    {
        PNTSETTINGS* pSettings = PanitentApp_GetSettings(pPanitentApp);
        if (pSettings)
        {
            hue = pSettings->iThemeHue;
            saturation = pSettings->iThemeSaturation;
            lightness = pSettings->iThemeLightness;
        }
    }

    PanitentTheme_GetColorsForHsl(hue, saturation, lightness, pColors);
}

void PanitentTheme_GetColorsForHsl(int hue, int saturation, int lightness, PanitentThemeColors* pColors)
{
    int borderSat = 0;
    int borderLight = 0;
    int activeSat = 0;
    int activeLight = 0;
    int rootSat = 0;
    int rootLight = 0;
    int menuSat = 0;
    int menuLight = 0;
    int gripSat = 0;
    int gripLight = 0;
    int gripDotSat = 0;
    int gripDotLight = 0;
    int guideSat = 0;
    int guideLight = 0;
    int guideBorderSat = 0;
    int guideBorderLight = 0;

    if (!pColors)
    {
        return;
    }

    hue = ((hue % 360) + 360) % 360;
    saturation = PanitentTheme_ClampInt(saturation, 0, 100);
    lightness = PanitentTheme_ClampInt(lightness, 0, 100);

    borderSat = PanitentTheme_ClampInt((int)(saturation * 0.55f), 0, 100);
    borderLight = PanitentTheme_ClampInt(lightness - 16, 0, 100);
    activeSat = PanitentTheme_ClampInt(saturation + 2, 0, 100);
    activeLight = PanitentTheme_ClampInt(lightness + 10, 0, 100);
    rootSat = PanitentTheme_ClampInt((int)(saturation * 0.75f), 0, 100);
    rootLight = PanitentTheme_ClampInt(lightness - 11, 0, 100);
    menuSat = PanitentTheme_ClampInt((int)(saturation * 0.90f), 0, 100);
    menuLight = PanitentTheme_ClampInt(lightness - 4, 0, 100);
    gripSat = PanitentTheme_ClampInt(max(4, saturation / 4), 0, 100);
    gripLight = PanitentTheme_ClampInt(lightness - 24, 0, 100);
    gripDotSat = PanitentTheme_ClampInt(max(6, saturation / 3), 0, 100);
    gripDotLight = PanitentTheme_ClampInt(lightness - 6, 0, 100);
    guideSat = PanitentTheme_ClampInt((int)(saturation * 0.70f), 0, 100);
    guideLight = PanitentTheme_ClampInt(lightness - 8, 0, 100);
    guideBorderSat = PanitentTheme_ClampInt((int)(saturation * 0.55f), 0, 100);
    guideBorderLight = PanitentTheme_ClampInt(lightness + 19, 0, 100);

    pColors->accent = PanitentTheme_HslToColor(hue, saturation, lightness);
    pColors->accentActive = PanitentTheme_HslToColor(hue, activeSat, activeLight);
    pColors->border = PanitentTheme_HslToColor(hue, borderSat, borderLight);
    pColors->rootBackground = PanitentTheme_HslToColor(hue, rootSat, rootLight);
    pColors->workspaceHeader = PanitentTheme_HslToColor(hue, PanitentTheme_ClampInt((int)(saturation * 0.85f), 0, 100), PanitentTheme_ClampInt(lightness - 8, 0, 100));
    pColors->menuBackground = PanitentTheme_HslToColor(hue, menuSat, menuLight);
    pColors->menuHot = PanitentTheme_HslToColor(hue, PanitentTheme_ClampInt(saturation + 8, 0, 100), PanitentTheme_ClampInt(lightness + 4, 0, 100));
    pColors->menuOpen = PanitentTheme_HslToColor(hue, PanitentTheme_ClampInt(saturation + 8, 0, 100), PanitentTheme_ClampInt(lightness + 8, 0, 100));
    pColors->splitterFill = PanitentTheme_HslToColor(hue, gripSat, gripLight);
    pColors->splitterGrip = PanitentTheme_HslToColor(hue, gripDotSat, gripDotLight);
    pColors->guideBackplate = PanitentTheme_HslToColor(hue, guideSat, guideLight);
    pColors->guideBackplateBorder = PanitentTheme_HslToColor(hue, guideBorderSat, guideBorderLight);
    pColors->text = RGB(0xFF, 0xFF, 0xFF);
}

void PanitentTheme_GetCaptionPalette(CaptionFramePalette* pPalette)
{
    PanitentThemeColors colors = { 0 };

    if (!pPalette)
    {
        return;
    }

    PanitentTheme_GetColors(&colors);
    pPalette->frameFill = colors.accent;
    pPalette->border = colors.border;
    pPalette->captionFill = colors.accent;
    pPalette->text = colors.text;
}

void PanitentTheme_RefreshApplication(void)
{
    PanitentApp* pPanitentApp = PanitentApp_Instance();
    if (!pPanitentApp)
    {
        return;
    }

    if (pPanitentApp->pPanitentWindow)
    {
        if (pPanitentApp->pPanitentWindow->m_pDockHostWindow)
        {
            DockHostWindow_RefreshTheme(pPanitentApp->pPanitentWindow->m_pDockHostWindow);
        }
        PanitentWindow_RefreshTheme(pPanitentApp->pPanitentWindow);
    }

    EnumThreadWindows(GetCurrentThreadId(), PanitentTheme_EnumThreadWindowsProc, 0);
}

static int PanitentTheme_ClampInt(int value, int minimum, int maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static COLORREF PanitentTheme_HslToColor(int hue, int saturation, int lightness)
{
    float h = (((float)((hue % 360) + 360)) / 360.0f);
    float s = ((float)PanitentTheme_ClampInt(saturation, 0, 100)) / 100.0f;
    float l = ((float)PanitentTheme_ClampInt(lightness, 0, 100)) / 100.0f;
    float r = l;
    float g = l;
    float b = l;

    if (s > 0.0001f)
    {
        float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
        float p = 2.0f * l - q;
        r = PanitentTheme_HueToRgb(p, q, h + (1.0f / 3.0f));
        g = PanitentTheme_HueToRgb(p, q, h);
        b = PanitentTheme_HueToRgb(p, q, h - (1.0f / 3.0f));
    }

    return RGB(
        (BYTE)(PanitentTheme_ClampInt((int)(r * 255.0f + 0.5f), 0, 255)),
        (BYTE)(PanitentTheme_ClampInt((int)(g * 255.0f + 0.5f), 0, 255)),
        (BYTE)(PanitentTheme_ClampInt((int)(b * 255.0f + 0.5f), 0, 255)));
}

static float PanitentTheme_HueToRgb(float p, float q, float t)
{
    if (t < 0.0f)
    {
        t += 1.0f;
    }
    if (t > 1.0f)
    {
        t -= 1.0f;
    }
    if (t < (1.0f / 6.0f))
    {
        return p + (q - p) * 6.0f * t;
    }
    if (t < 0.5f)
    {
        return q;
    }
    if (t < (2.0f / 3.0f))
    {
        return p + (q - p) * ((2.0f / 3.0f) - t) * 6.0f;
    }
    return p;
}

static BOOL CALLBACK PanitentTheme_EnumThreadWindowsProc(HWND hWnd, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    return TRUE;
}

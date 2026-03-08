#ifndef PANITENT_SETTINGS_H_
#define PANITENT_SETTINGS_H_

#include "util.h"

typedef struct _tagPNTSETTINGS {
    int x;
    int y;
    int width;
    int height;
    BOOL bRememberWindowPos;
    BOOL bLegacyFileDialogs;
    BOOL bEnablePenTablet;
    int iToolbarIconTheme;
    BOOL bUseStandardWindowFrame;
    BOOL bCompactMenuBar;
    int iThemeHue;
    int iThemeSaturation;
    int iThemeLightness;
    int iRecoveryRetentionHours;
} PNTSETTINGS;

void Panitent_DefaultSettings(PNTSETTINGS* pSettings);
BOOL Panitent_ReadSettings(PNTSETTINGS* pSettings);
BOOL Panitent_WriteSettings(const PNTSETTINGS* pSettings);

#endif /* PANITENT_SETTINGS_H_ */

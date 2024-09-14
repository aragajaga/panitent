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
} PNTSETTINGS;

#endif /* PANITENT_SETTINGS_H_ */

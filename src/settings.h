#ifndef PANITENT_SETTINGS_H_
#define PANITENT_SETTINGS_H_

#include "util.h"

BOOL SettingsWindow_Register(HINSTANCE);
int ShowSettingsWindow(HWND hwnd);

typedef struct _tagPNTSETTINGS {
  int x;
  int y;
  int width;
  int height;
  BOOL bRememberWindowPos;
  BOOL bLegacyFileDialogs;
  BOOL bEnablePenTablet;
} PNTSETTINGS;

#endif /* PANITENT_SETTINGS_H_ */

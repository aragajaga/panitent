#ifndef PANITENT_SETTINGS_H_
#define PANITENT_SETTINGS_H_

#include "util.h"

BOOL SettingsWindow_Register(HINSTANCE);
int ShowSettingsWindow(HWND hwnd);

typedef struct _tagPNTSETTINGS {
  DWORD x;
  DWORD y;
  DWORD width;
  DWORD height;
  BOOL bRememberWindowPos;
  BOOL bLegacyFileDialogs;
} PNTSETTINGS;

#endif /* PANITENT_SETTINGS_H_ */

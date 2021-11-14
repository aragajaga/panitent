#ifndef PANITENT_OPTION_BAR_H_
#define PANITENT_OPTION_BAR_H_

#include "precomp.h"

typedef struct _OptionBar {
  ATOM win_class;
  HWND win_handle;
  HWND textstring_handle;
} OptionBar;

extern OptionBar g_option_bar;

BOOL BrushSel_RegisterClass(HINSTANCE hInstance);
BOOL OptionBar_RegisterClass(HINSTANCE hInstance);

#endif /* PANITENT_OPTION_BAR_H_ */

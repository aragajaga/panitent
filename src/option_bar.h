#ifndef PANITENT_OPTION_BAR_H_
#define PANITENT_OPTION_BAR_H_

#include "precomp.h"

typedef struct _OptionBar {
  ATOM win_class;
  HWND win_handle;
  HWND textstring_handle;
} OptionBar;

extern OptionBar g_option_bar;

void OptionBar_RegisterClass(HINSTANCE);
void OptionBar_Create(HWND);

#endif /* PANITENT_OPTION_BAR_H_ */

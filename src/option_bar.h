#ifndef PANITENT_OPTION_BAR_H_
#define PANITENT_OPTION_BAR_H_

#include "precomp.h"

typedef struct _option_bar {
  ATOM win_class;
  HWND win_handle;
  HWND textstring_handle;
} option_bar_t;

extern option_bar_t g_option_bar;

void option_bar_register_class(HINSTANCE);
void option_bar_create(HWND);

#endif /* PANITENT_OPTION_BAR_H_ */

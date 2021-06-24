#ifndef PANITENT_SWATCH2_H_
#define PANITENT_SWATCH2_H_

#include "precomp.h"

#include <stdint.h>

#define SCN_COLORCHANGE WM_USER+1
#define SCM_GETCOLOR WM_USER+2
#define SCM_SETCOLOR WM_USER+3

#define WC_SWATCHCONTROL2 L"Win32Class_SwatchControl2"

BOOL SwatchControl2_RegisterClass(HINSTANCE hInstance);

#define SwatchControl2_GetColor(hwnd) \
  ((uint32_t)(DWORD)SendMessage((hwnd), SCM_GETCOLOR, 0L, 0L))
#define SwatchControl2_SetColor(hwnd, color) \
  ((void)SendMessage((hwnd), SCM_SETCOLOR, (WPARAM)(uint32_t)(color), 0L))

#endif  /* PANITENT_SWATCH2_H_ */

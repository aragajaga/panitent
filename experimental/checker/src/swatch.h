#ifndef CHECKER_SWATCH_H_
#define CHECKER_SWATCH_H_

#include <stdint.h>
#include "precomp.h"

#define SCN_COLORCHANGE WM_USER+1
#define SCM_GETCOLOR WM_USER+2

LRESULT CALLBACK SwatchControl_WndProc(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData);
HWND SwatchControl_Create(uint32_t color, int x, int y, int width,
    int height, HWND hParent, HMENU hMenu);

#endif  /* CHECKER_SWATCH_H_ */

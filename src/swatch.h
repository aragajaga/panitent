#ifndef CHECKER_SWATCH_H_
#define CHECKER_SWATCH_H_

#include "precomp.h"

#include <stdint.h>

LRESULT CALLBACK SwatchControl_WndProc(HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData);
HWND SwatchControl_Create(uint32_t color, int x, int y, int width,
    int height, HWND hParent);

#endif  /* CHECKER_SWATCH_H_ */

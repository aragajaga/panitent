#pragma once

#include "floatingtoolhost.h"

BOOL FloatingToolHost_IsPinnedFloatingWindow(HWND hWnd, FloatingWindowContainer** ppFloatingWindowContainer);
BOOL FloatingToolHost_ForEachPinnedWindow(FnFloatingToolHostWindowCallback pfnCallback, void* pUserData);
void FloatingToolHost_DestroyExistingPinnedWindows(void);
BOOL FloatingToolHost_CapturePinnedWindowState(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    DockFloatingLayoutEntry* pEntry);

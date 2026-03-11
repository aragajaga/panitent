#pragma once

#include "dockhost.h"
#include "toolwndframe.h"

void DockHostWindow_SetAutoHideHotButton(DockHostWindow* pDockHostWindow, int nButton);
void DockHostWindow_SetAutoHidePressedButton(DockHostWindow* pDockHostWindow, int nButton);
void DockHostWindow_ClearAutoHideCaptionState(DockHostWindow* pDockHostWindow);
BOOL DockHostWindow_GetAutoHideOverlayRect(DockHostWindow* pDockHostWindow, int nDockSide, RECT* pRect);
BOOL DockHostWindow_BuildAutoHideOverlayLayout(DockHostWindow* pDockHostWindow, CaptionFrameLayout* pLayout);
void DockHostWindow_HideAutoHideOverlay(DockHostWindow* pDockHostWindow);
void DockHostWindow_UpdateAutoHideOverlay(DockHostWindow* pDockHostWindow);
BOOL DockHostWindow_ShowAutoHideOverlay(DockHostWindow* pDockHostWindow, int nDockSide, HWND hWndTab);

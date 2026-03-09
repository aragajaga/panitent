#pragma once

#include "dockhost.h"

int DockHostPaint_HitTestZoneTab(DockHostWindow* pDockHostWindow, int x, int y, HWND* phWndTab, int iZoneTabGutter);
void DockHostPaint_PaintContent(
    DockHostWindow* pDockHostWindow,
    HDC hdc,
    const RECT* pClientRect,
    HBRUSH hCaptionBrush,
    int iZoneTabGutter,
    int iBorderWidth);

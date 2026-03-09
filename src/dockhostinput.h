#pragma once

#include "dockhost.h"

void DockHostInput_InvokeInspectorDialog(DockHostWindow* pDockHostWindow);
void DockHostInput_ClearCaptionState(DockHostWindow* pDockHostWindow);
void DockHostInput_OnMouseLeave(DockHostWindow* pDockHostWindow);
void DockHostInput_OnCaptureChanged(DockHostWindow* pDockHostWindow);
void DockHostInput_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags, int iZoneTabGutter);
void DockHostInput_OnLButtonDown(DockHostWindow* pDockHostWindow, BOOL fDoubleClick, int x, int y, UINT keyFlags, int iZoneTabGutter);
void DockHostInput_OnLButtonUp(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags);
void DockHostInput_OnContextMenu(DockHostWindow* pDockHostWindow, HWND hWndContext, int x, int y);

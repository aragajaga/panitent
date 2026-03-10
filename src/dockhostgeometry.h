#pragma once

#include "dockhost.h"

BOOL DockData_GetClientRect(DockData* pDockData, RECT* rc);
void DockData_Init(DockData* pDockData);
BOOL DockData_GetCaptionRect(DockData* pDockData, RECT* rc);
BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect);

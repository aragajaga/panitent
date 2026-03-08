#pragma once

#include "precomp.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;

BOOL PanitentDockLayout_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);
BOOL PanitentDockLayout_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const RECT* pDockHostRect);

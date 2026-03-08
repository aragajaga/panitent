#pragma once

#include "precomp.h"

#include "dockhost.h"
#include "dockfloatingmodel.h"

typedef struct PanitentApp PanitentApp;
BOOL PanitentDockFloating_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);
BOOL PanitentDockFloating_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);

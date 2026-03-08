#pragma once

#include "precomp.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;

BOOL PanitentFloatingDocumentSession_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);
BOOL PanitentFloatingDocumentSession_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);

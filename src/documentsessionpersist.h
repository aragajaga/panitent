#pragma once

#include "precomp.h"

typedef struct PanitentApp PanitentApp;

BOOL PanitentDocumentSession_Save(PanitentApp* pPanitentApp);
BOOL PanitentDocumentSession_Restore(PanitentApp* pPanitentApp);

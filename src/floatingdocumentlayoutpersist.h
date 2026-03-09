#pragma once

#include "precomp.h"

#include "floatingdocumentlayoutmodel.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;

BOOL PanitentFloatingDocumentLayout_CaptureModel(PanitentApp* pPanitentApp, FloatingDocumentLayoutModel* pModel);
BOOL PanitentFloatingDocumentLayout_RestoreModel(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel);

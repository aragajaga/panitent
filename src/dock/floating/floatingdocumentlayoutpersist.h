#pragma once

#include "precomp.h"

#include "floatingdocumentlayoutmodel.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;
typedef struct FloatingDocumentLayoutEntry FloatingDocumentLayoutEntry;
typedef BOOL (*FnFloatingDocumentLayoutRestoreEntryTestHook)(const FloatingDocumentLayoutEntry* pEntry);

BOOL PanitentFloatingDocumentLayout_CaptureModel(PanitentApp* pPanitentApp, FloatingDocumentLayoutModel* pModel);
BOOL PanitentFloatingDocumentLayout_RestoreModel(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel);
BOOL PanitentFloatingDocumentLayout_RestoreModelEx(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	const FloatingDocumentLayoutModel* pModel,
	BOOL bRequireAllEntries);
void PanitentFloatingDocumentLayout_SetRestoreEntryTestHook(FnFloatingDocumentLayoutRestoreEntryTestHook pfnHook);

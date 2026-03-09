#pragma once

#include "precomp.h"

#include "dockhost.h"
#include "dockfloatingmodel.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockFloatingLayoutFileModel DockFloatingLayoutFileModel;
typedef struct DockFloatingLayoutEntry DockFloatingLayoutEntry;
typedef BOOL (*FnDockFloatingRestoreEntryTestHook)(const DockFloatingLayoutEntry* pEntry);
BOOL PanitentDockFloating_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);
BOOL PanitentDockFloating_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow);
BOOL PanitentDockFloating_SaveToFilePath(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, PCWSTR pszFilePath);
BOOL PanitentDockFloating_RestoreFromFilePath(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, PCWSTR pszFilePath);
BOOL PanitentDockFloating_CaptureModel(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, DockFloatingLayoutFileModel* pModel);
BOOL PanitentDockFloating_RestoreModel(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const DockFloatingLayoutFileModel* pModel);
BOOL PanitentDockFloating_RestoreModelEx(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const DockFloatingLayoutFileModel* pModel, BOOL bRequireAllEntries);
void PanitentDockFloating_SetRestoreEntryTestHook(FnDockFloatingRestoreEntryTestHook pfnHook);

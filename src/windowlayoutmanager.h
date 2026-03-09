#pragma once

#include "precomp.h"

typedef struct PanitentWindow PanitentWindow;
typedef struct DockModelNode DockModelNode;
typedef struct DockFloatingLayoutFileModel DockFloatingLayoutFileModel;
typedef struct FloatingDocumentLayoutModel FloatingDocumentLayoutModel;

void WindowLayoutManager_RefreshApplyMenu(PanitentWindow* pPanitentWindow);
BOOL WindowLayoutManager_HandleCommand(PanitentWindow* pPanitentWindow, UINT cmdId);
BOOL WindowLayoutManager_ApplyLayoutBundle(
    PanitentWindow* pPanitentWindow,
    DockModelNode* pModelRoot,
    const DockFloatingLayoutFileModel* pFloatingModel,
    const FloatingDocumentLayoutModel* pFloatDocModel);
BOOL WindowLayoutManager_ApplyDefaultLayout(PanitentWindow* pPanitentWindow);

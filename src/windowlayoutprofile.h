#pragma once

#include "precomp.h"

#include "dockfloatingmodel.h"
#include "dockmodel.h"
#include "floatingdocumentlayoutmodel.h"

BOOL WindowLayoutProfile_GetDockLayoutPath(uint32_t uId, PTSTR* ppszPath);
BOOL WindowLayoutProfile_GetDockFloatingPath(uint32_t uId, PTSTR* ppszPath);
BOOL WindowLayoutProfile_GetFloatDocLayoutPath(uint32_t uId, PTSTR* ppszPath);

BOOL WindowLayoutProfile_SaveBundle(
    uint32_t uId,
    const DockModelNode* pDockLayoutModel,
    const DockFloatingLayoutFileModel* pDockFloatingModel,
    const FloatingDocumentLayoutModel* pFloatDocLayoutModel);

BOOL WindowLayoutProfile_LoadBundle(
    uint32_t uId,
    DockModelNode** ppDockLayoutModel,
    DockFloatingLayoutFileModel* pDockFloatingModel,
    FloatingDocumentLayoutModel* pFloatDocLayoutModel);

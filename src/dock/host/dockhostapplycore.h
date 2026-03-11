#pragma once

#include "dockhostpreserve.h"

DockModelNode* DockHostApplyCore_CreatePanelModel(HWND hWnd);
DockModelNode* DockHostApplyCore_CreateWorkspaceModel(HWND hWnd);
BOOL DockHostApplyCore_ApplyModel(DockHostWindow* pDockHostWindow, DockModelNode* pModelRoot, DockHostModelApplyContext* pContext);

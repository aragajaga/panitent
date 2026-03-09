#pragma once

#include "dockmodel.h"

DockModelNode* DockModelOps_CloneTree(const DockModelNode* pRootNode);
DockModelNode* DockModelOps_FindByNodeId(DockModelNode* pRootNode, uint32_t uNodeId);
DockModelNode* DockModelOps_FindParentByNodeId(DockModelNode* pRootNode, uint32_t uNodeId);
uint32_t DockModelOps_GetMaxNodeId(const DockModelNode* pRootNode);
BOOL DockModelOps_RemoveNodeById(DockModelNode** ppRootNode, uint32_t uNodeId);
BOOL DockModelOps_AppendPanelToZone(DockModelNode* pRootNode, int nDockSide, DockModelNode* pPanelNode);

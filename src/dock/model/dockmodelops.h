#pragma once

#include "dockmodel.h"

DockModelNode* DockModelOps_CloneTree(const DockModelNode* pRootNode);
DockModelNode* DockModelOps_FindByNodeId(DockModelNode* pRootNode, uint32_t uNodeId);
DockModelNode* DockModelOps_FindParentByNodeId(DockModelNode* pRootNode, uint32_t uNodeId);
uint32_t DockModelOps_GetMaxNodeId(const DockModelNode* pRootNode);
BOOL DockModelOps_RemoveNodeById(DockModelNode** ppRootNode, uint32_t uNodeId);
BOOL DockModelOps_AppendPanelToZone(DockModelNode* pRootNode, int nDockSide, DockModelNode* pPanelNode);
BOOL DockModelOps_DockLeafAroundNode(DockModelNode* pRootNode, uint32_t uAnchorNodeId, int nDockSide, DockModelNode* pLeafNode);
BOOL DockModelOps_DockPanelAroundNode(DockModelNode* pRootNode, uint32_t uAnchorNodeId, int nDockSide, DockModelNode* pPanelNode);
BOOL DockModelOps_DockWorkspaceAroundNode(DockModelNode* pRootNode, uint32_t uAnchorNodeId, int nDockSide, DockModelNode* pWorkspaceNode);
BOOL DockModelOps_DockPanelAtRootSide(DockModelNode* pRootNode, int nDockSide, DockModelNode* pPanelNode);

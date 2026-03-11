#pragma once

#include "dockmodel.h"

TreeNode* DockModelBuildTree(const DockModelNode* pRootModel);
void DockModelBuildDestroyTree(TreeNode* pRootNode);

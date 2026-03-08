#pragma once

#include "dockmodel.h"

BOOL DockModelIO_SaveToFile(const DockModelNode* pRootNode, PCWSTR pszFilePath);
DockModelNode* DockModelIO_LoadFromFile(PCWSTR pszFilePath);

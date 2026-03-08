#pragma once

#include <stdio.h>

#include "dockmodel.h"

BOOL DockModelIO_WriteToStream(FILE* fp, const DockModelNode* pRootNode);
DockModelNode* DockModelIO_ReadFromStream(FILE* fp);
BOOL DockModelIO_SaveToFile(const DockModelNode* pRootNode, PCWSTR pszFilePath);
DockModelNode* DockModelIO_LoadFromFile(PCWSTR pszFilePath);

#pragma once

#include <stdio.h>

#include "dockmodel.h"
#include "persistload.h"

BOOL DockModelIO_WriteToStream(FILE* fp, const DockModelNode* pRootNode);
DockModelNode* DockModelIO_ReadFromStream(FILE* fp);
DockModelNode* DockModelIO_LoadFromFileEx(PCWSTR pszFilePath, PersistLoadStatus* pStatus);
BOOL DockModelIO_SaveToFile(const DockModelNode* pRootNode, PCWSTR pszFilePath);
DockModelNode* DockModelIO_LoadFromFile(PCWSTR pszFilePath);

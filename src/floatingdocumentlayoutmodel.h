#pragma once

#include "precomp.h"

#include "dockmodel.h"
#include "persistload.h"

typedef struct FloatingDocumentLayoutEntry
{
	RECT rcWindow;
	DockModelNode* pLayoutModel;
} FloatingDocumentLayoutEntry;

typedef struct FloatingDocumentLayoutModel
{
	int nEntryCount;
	FloatingDocumentLayoutEntry entries[16];
} FloatingDocumentLayoutModel;

BOOL FloatingDocumentLayoutModel_SaveToFile(const FloatingDocumentLayoutModel* pModel, PCWSTR pszFilePath);
BOOL FloatingDocumentLayoutModel_LoadFromFileEx(PCWSTR pszFilePath, FloatingDocumentLayoutModel* pModel, PersistLoadStatus* pStatus);
BOOL FloatingDocumentLayoutModel_LoadFromFile(PCWSTR pszFilePath, FloatingDocumentLayoutModel* pModel);
void FloatingDocumentLayoutModel_Destroy(FloatingDocumentLayoutModel* pModel);

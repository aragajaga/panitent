#pragma once

#include "precomp.h"

typedef struct FloatingDocumentSessionEntry
{
	RECT rcWindow;
	int nActiveEntry;
	int nFileCount;
	WCHAR szFilePaths[32][MAX_PATH];
} FloatingDocumentSessionEntry;

typedef struct FloatingDocumentSessionModel
{
	int nEntryCount;
	FloatingDocumentSessionEntry entries[16];
} FloatingDocumentSessionModel;

BOOL FloatingDocumentSessionModel_SaveToFile(const FloatingDocumentSessionModel* pModel, PCWSTR pszFilePath);
BOOL FloatingDocumentSessionModel_LoadFromFile(PCWSTR pszFilePath, FloatingDocumentSessionModel* pModel);

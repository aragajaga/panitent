#pragma once

#include "precomp.h"

typedef struct DocumentSessionEntry
{
	WCHAR szFilePath[MAX_PATH];
} DocumentSessionEntry;

typedef struct DocumentSessionModel
{
	int nEntryCount;
	int nActiveEntry;
	DocumentSessionEntry entries[64];
} DocumentSessionModel;

BOOL DocumentSessionModel_SaveToFile(const DocumentSessionModel* pModel, PCWSTR pszFilePath);
BOOL DocumentSessionModel_LoadFromFile(PCWSTR pszFilePath, DocumentSessionModel* pModel);

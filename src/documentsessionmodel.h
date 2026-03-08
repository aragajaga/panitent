#pragma once

#include "precomp.h"
#include "persistload.h"

typedef enum DocumentSessionEntryKind
{
	DOCSESSION_ENTRY_NONE = 0,
	DOCSESSION_ENTRY_FILE = 1,
	DOCSESSION_ENTRY_RECOVERY = 2
} DocumentSessionEntryKind;

typedef struct DocumentSessionEntry
{
	int nKind;
	WCHAR szFilePath[MAX_PATH];
} DocumentSessionEntry;

typedef struct DocumentSessionModel
{
	int nEntryCount;
	int nActiveEntry;
	DocumentSessionEntry entries[64];
} DocumentSessionModel;

BOOL DocumentSessionModel_SaveToFile(const DocumentSessionModel* pModel, PCWSTR pszFilePath);
BOOL DocumentSessionModel_LoadFromFileEx(PCWSTR pszFilePath, DocumentSessionModel* pModel, PersistLoadStatus* pStatus);
BOOL DocumentSessionModel_LoadFromFile(PCWSTR pszFilePath, DocumentSessionModel* pModel);

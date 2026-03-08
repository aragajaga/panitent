#pragma once

#include "precomp.h"
#include "dockmodel.h"
#include "documentsessionmodel.h"
#include "persistload.h"

typedef struct FloatingDocumentWorkspaceSession
{
	int nActiveEntry;
	int nFileCount;
	DocumentSessionEntry entries[32];
} FloatingDocumentWorkspaceSession;

typedef struct FloatingDocumentSessionEntry
{
	RECT rcWindow;
	DockModelNode* pLayoutModel;
	int nWorkspaceCount;
	FloatingDocumentWorkspaceSession workspaces[16];
} FloatingDocumentSessionEntry;

typedef struct FloatingDocumentSessionModel
{
	int nEntryCount;
	FloatingDocumentSessionEntry entries[16];
} FloatingDocumentSessionModel;

BOOL FloatingDocumentSessionModel_SaveToFile(const FloatingDocumentSessionModel* pModel, PCWSTR pszFilePath);
BOOL FloatingDocumentSessionModel_LoadFromFileEx(PCWSTR pszFilePath, FloatingDocumentSessionModel* pModel, PersistLoadStatus* pStatus);
BOOL FloatingDocumentSessionModel_LoadFromFile(PCWSTR pszFilePath, FloatingDocumentSessionModel* pModel);
void FloatingDocumentSessionModel_Destroy(FloatingDocumentSessionModel* pModel);

#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "floatingdocumentsessionmodel.h"
#include "dockmodelio.h"

#define FLOATDOCSESSION_MAGIC 0x53444644u /* DFDS */
#define FLOATDOCSESSION_VERSION 2u

typedef struct FloatingDocumentSessionFileHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t count;
} FloatingDocumentSessionFileHeader;

typedef struct LegacyFloatingDocumentSessionEntryV1
{
	RECT rcWindow;
	int nActiveEntry;
	int nFileCount;
	WCHAR szFilePaths[32][MAX_PATH];
} LegacyFloatingDocumentSessionEntryV1;

static DockModelNode* FloatingDocumentSessionModel_CreateLegacyWorkspaceLayout(void)
{
	DockModelNode* pRoot = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	DockModelNode* pWorkspace = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	if (!pRoot || !pWorkspace)
	{
		free(pRoot);
		free(pWorkspace);
		return NULL;
	}

	pRoot->nRole = DOCK_ROLE_ROOT;
	wcscpy_s(pRoot->szName, ARRAYSIZE(pRoot->szName), L"Root");
	pRoot->pChild1 = pWorkspace;

	pWorkspace->nRole = DOCK_ROLE_WORKSPACE;
	pWorkspace->nPaneKind = DOCK_PANE_DOCUMENT;
	wcscpy_s(pWorkspace->szName, ARRAYSIZE(pWorkspace->szName), L"WorkspaceContainer");
	return pRoot;
}

BOOL FloatingDocumentSessionModel_SaveToFile(const FloatingDocumentSessionModel* pModel, PCWSTR pszFilePath)
{
	FILE* fp = NULL;
	FloatingDocumentSessionFileHeader header = { FLOATDOCSESSION_MAGIC, FLOATDOCSESSION_VERSION, 0 };
	if (!pModel || !pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"wb") != 0 || !fp)
	{
		return FALSE;
	}

	header.count = (uint32_t)max(0, min(pModel->nEntryCount, (int)ARRAYSIZE(pModel->entries)));
	BOOL bOk = fwrite(&header, sizeof(header), 1, fp) == 1;
	if (bOk && header.count > 0)
	{
		for (uint32_t i = 0; i < header.count && bOk; ++i)
		{
			const FloatingDocumentSessionEntry* pEntry = &pModel->entries[i];
			bOk =
				fwrite(&pEntry->rcWindow, sizeof(pEntry->rcWindow), 1, fp) == 1 &&
				fwrite(&pEntry->nWorkspaceCount, sizeof(pEntry->nWorkspaceCount), 1, fp) == 1 &&
				fwrite(pEntry->workspaces, sizeof(FloatingDocumentWorkspaceSession), ARRAYSIZE(pEntry->workspaces), fp) == ARRAYSIZE(pEntry->workspaces) &&
				DockModelIO_WriteToStream(fp, pEntry->pLayoutModel);
		}
	}

	fclose(fp);
	return bOk;
}

BOOL FloatingDocumentSessionModel_LoadFromFile(PCWSTR pszFilePath, FloatingDocumentSessionModel* pModel)
{
	return FloatingDocumentSessionModel_LoadFromFileEx(pszFilePath, pModel, NULL);
}

BOOL FloatingDocumentSessionModel_LoadFromFileEx(PCWSTR pszFilePath, FloatingDocumentSessionModel* pModel, PersistLoadStatus* pStatus)
{
	FILE* fp = NULL;
	FloatingDocumentSessionFileHeader header = { 0 };
	if (!pModel)
	{
		return FALSE;
	}

	if (pStatus)
	{
		*pStatus = PERSIST_LOAD_IO_ERROR;
	}

	memset(pModel, 0, sizeof(*pModel));
	if (!pszFilePath || !pszFilePath[0])
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_NOT_FOUND;
		}
		return FALSE;
	}

	DWORD dwAttrs = GetFileAttributesW(pszFilePath);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES)
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_NOT_FOUND;
		}
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"rb") != 0 || !fp)
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_IO_ERROR;
		}
		return FALSE;
	}

	BOOL bOk = fread(&header, sizeof(header), 1, fp) == 1 &&
		header.magic == FLOATDOCSESSION_MAGIC &&
		(header.version == 1u || header.version == FLOATDOCSESSION_VERSION) &&
		header.count <= ARRAYSIZE(pModel->entries);
	if (bOk && header.count > 0)
	{
		if (header.version == 1u)
		{
			LegacyFloatingDocumentSessionEntryV1 legacyEntries[16] = { 0 };
			bOk = fread(legacyEntries, sizeof(LegacyFloatingDocumentSessionEntryV1), header.count, fp) == header.count;
			if (bOk)
			{
				for (uint32_t i = 0; i < header.count && bOk; ++i)
				{
					FloatingDocumentSessionEntry* pEntry = &pModel->entries[i];
					pEntry->rcWindow = legacyEntries[i].rcWindow;
					pEntry->pLayoutModel = FloatingDocumentSessionModel_CreateLegacyWorkspaceLayout();
					if (!pEntry->pLayoutModel)
					{
						bOk = FALSE;
						break;
					}
					pEntry->nWorkspaceCount = 1;
					pEntry->workspaces[0].nActiveEntry = legacyEntries[i].nActiveEntry;
					pEntry->workspaces[0].nFileCount = legacyEntries[i].nFileCount;
					for (int j = 0; j < legacyEntries[i].nFileCount && j < ARRAYSIZE(pEntry->workspaces[0].entries); ++j)
					{
						pEntry->workspaces[0].entries[j].nKind = DOCSESSION_ENTRY_FILE;
						wcscpy_s(
							pEntry->workspaces[0].entries[j].szFilePath,
							ARRAYSIZE(pEntry->workspaces[0].entries[j].szFilePath),
							legacyEntries[i].szFilePaths[j]);
					}
				}
			}
		}
		else {
			for (uint32_t i = 0; i < header.count && bOk; ++i)
			{
				FloatingDocumentSessionEntry* pEntry = &pModel->entries[i];
				bOk =
					fread(&pEntry->rcWindow, sizeof(pEntry->rcWindow), 1, fp) == 1 &&
					fread(&pEntry->nWorkspaceCount, sizeof(pEntry->nWorkspaceCount), 1, fp) == 1 &&
					fread(pEntry->workspaces, sizeof(FloatingDocumentWorkspaceSession), ARRAYSIZE(pEntry->workspaces), fp) == ARRAYSIZE(pEntry->workspaces);
				if (bOk)
				{
					pEntry->pLayoutModel = DockModelIO_ReadFromStream(fp);
					bOk = pEntry->pLayoutModel != NULL;
				}
			}
		}
	}

	fclose(fp);
	if (bOk)
	{
		pModel->nEntryCount = (int)header.count;
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_OK;
		}
	}
	else if (pStatus)
	{
		*pStatus = PERSIST_LOAD_INVALID_FORMAT;
	}

	return bOk;
}

void FloatingDocumentSessionModel_Destroy(FloatingDocumentSessionModel* pModel)
{
	if (!pModel)
	{
		return;
	}

	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		DockModel_Destroy(pModel->entries[i].pLayoutModel);
		pModel->entries[i].pLayoutModel = NULL;
	}

	pModel->nEntryCount = 0;
}

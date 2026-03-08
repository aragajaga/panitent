#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "floatingdocumentsessionmodel.h"
#include "dockmodelio.h"

#define FLOATDOCSESSION_MAGIC 0x53444644u /* DFDS */
#define FLOATDOCSESSION_VERSION 1u

typedef struct FloatingDocumentSessionFileHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t count;
} FloatingDocumentSessionFileHeader;

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
	FILE* fp = NULL;
	FloatingDocumentSessionFileHeader header = { 0 };
	if (!pModel)
	{
		return FALSE;
	}

	memset(pModel, 0, sizeof(*pModel));
	if (!pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"rb") != 0 || !fp)
	{
		return FALSE;
	}

	BOOL bOk = fread(&header, sizeof(header), 1, fp) == 1 &&
		header.magic == FLOATDOCSESSION_MAGIC &&
		header.version == FLOATDOCSESSION_VERSION &&
		header.count <= ARRAYSIZE(pModel->entries);
	if (bOk && header.count > 0)
	{
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

	fclose(fp);
	if (bOk)
	{
		pModel->nEntryCount = (int)header.count;
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

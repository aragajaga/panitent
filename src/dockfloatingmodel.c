#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "dockfloatingmodel.h"
#include "dockmodelio.h"

#define DOCKFLOATING_MAGIC 0x474C4644u /* DFLG */
#define DOCKFLOATING_VERSION 1u

typedef struct DockFloatingFileHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t count;
} DockFloatingFileHeader;

BOOL DockFloatingLayout_SaveToFile(const DockFloatingLayoutFileModel* pModel, PCWSTR pszFilePath)
{
	FILE* fp = NULL;
	DockFloatingFileHeader header = { DOCKFLOATING_MAGIC, DOCKFLOATING_VERSION, 0 };
	if (!pModel || !pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"wb") != 0 || !fp)
	{
		return FALSE;
	}

	header.count = (uint32_t)max(0, min(pModel->nEntries, (int)ARRAYSIZE(pModel->entries)));
	BOOL bOk = fwrite(&header, sizeof(header), 1, fp) == 1;
	if (bOk && header.count > 0)
	{
		for (uint32_t i = 0; i < header.count && bOk; ++i)
		{
			const DockFloatingLayoutEntry* pEntry = &pModel->entries[i];
			bOk =
				fwrite(&pEntry->rcWindow, sizeof(pEntry->rcWindow), 1, fp) == 1 &&
				fwrite(&pEntry->iDockSizeHint, sizeof(pEntry->iDockSizeHint), 1, fp) == 1 &&
				fwrite(&pEntry->nChildKind, sizeof(pEntry->nChildKind), 1, fp) == 1 &&
				fwrite(&pEntry->nViewId, sizeof(pEntry->nViewId), 1, fp) == 1;
			if (bOk)
			{
				uint8_t bHasLayoutModel = pEntry->pLayoutModel ? 1 : 0;
				bOk = fwrite(&bHasLayoutModel, sizeof(bHasLayoutModel), 1, fp) == 1;
				if (bOk && bHasLayoutModel)
				{
					bOk = DockModelIO_WriteToStream(fp, pEntry->pLayoutModel);
				}
			}
		}
	}

	fclose(fp);
	return bOk;
}

BOOL DockFloatingLayout_LoadFromFile(PCWSTR pszFilePath, DockFloatingLayoutFileModel* pModel)
{
	return DockFloatingLayout_LoadFromFileEx(pszFilePath, pModel, NULL);
}

BOOL DockFloatingLayout_LoadFromFileEx(PCWSTR pszFilePath, DockFloatingLayoutFileModel* pModel, PersistLoadStatus* pStatus)
{
	FILE* fp = NULL;
	DockFloatingFileHeader header = { 0 };
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
		header.magic == DOCKFLOATING_MAGIC &&
		header.version == DOCKFLOATING_VERSION &&
		header.count <= ARRAYSIZE(pModel->entries);
	if (bOk && header.count > 0)
	{
		for (uint32_t i = 0; i < header.count && bOk; ++i)
		{
			DockFloatingLayoutEntry* pEntry = &pModel->entries[i];
			bOk =
				fread(&pEntry->rcWindow, sizeof(pEntry->rcWindow), 1, fp) == 1 &&
				fread(&pEntry->iDockSizeHint, sizeof(pEntry->iDockSizeHint), 1, fp) == 1 &&
				fread(&pEntry->nChildKind, sizeof(pEntry->nChildKind), 1, fp) == 1 &&
				fread(&pEntry->nViewId, sizeof(pEntry->nViewId), 1, fp) == 1;
			if (bOk)
			{
				uint8_t bHasLayoutModel = 0;
				bOk = fread(&bHasLayoutModel, sizeof(bHasLayoutModel), 1, fp) == 1;
				if (bOk && bHasLayoutModel)
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
		pModel->nEntries = (int)header.count;
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

void DockFloatingLayout_Destroy(DockFloatingLayoutFileModel* pModel)
{
	if (!pModel)
	{
		return;
	}

	for (int i = 0; i < pModel->nEntries; ++i)
	{
		DockModel_Destroy(pModel->entries[i].pLayoutModel);
		pModel->entries[i].pLayoutModel = NULL;
	}

	pModel->nEntries = 0;
}

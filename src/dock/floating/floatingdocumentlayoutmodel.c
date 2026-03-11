#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "floatingdocumentlayoutmodel.h"
#include "dockmodelio.h"

#define FLOATDOCLAYOUT_MAGIC 0x4C444644u /* DFDL */
#define FLOATDOCLAYOUT_VERSION 1u

typedef struct FloatingDocumentLayoutFileHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t count;
} FloatingDocumentLayoutFileHeader;

BOOL FloatingDocumentLayoutModel_SaveToFile(const FloatingDocumentLayoutModel* pModel, PCWSTR pszFilePath)
{
	FILE* fp = NULL;
	FloatingDocumentLayoutFileHeader header = { FLOATDOCLAYOUT_MAGIC, FLOATDOCLAYOUT_VERSION, 0 };
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
			const FloatingDocumentLayoutEntry* pEntry = &pModel->entries[i];
			bOk =
				fwrite(&pEntry->rcWindow, sizeof(pEntry->rcWindow), 1, fp) == 1 &&
				DockModelIO_WriteToStream(fp, pEntry->pLayoutModel);
		}
	}

	fclose(fp);
	return bOk;
}

BOOL FloatingDocumentLayoutModel_LoadFromFile(PCWSTR pszFilePath, FloatingDocumentLayoutModel* pModel)
{
	return FloatingDocumentLayoutModel_LoadFromFileEx(pszFilePath, pModel, NULL);
}

BOOL FloatingDocumentLayoutModel_LoadFromFileEx(PCWSTR pszFilePath, FloatingDocumentLayoutModel* pModel, PersistLoadStatus* pStatus)
{
	FILE* fp = NULL;
	FloatingDocumentLayoutFileHeader header = { 0 };
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
		header.magic == FLOATDOCLAYOUT_MAGIC &&
		header.version == FLOATDOCLAYOUT_VERSION &&
		header.count <= ARRAYSIZE(pModel->entries);
	if (bOk && header.count > 0)
	{
		for (uint32_t i = 0; i < header.count && bOk; ++i)
		{
			FloatingDocumentLayoutEntry* pEntry = &pModel->entries[i];
			bOk = fread(&pEntry->rcWindow, sizeof(pEntry->rcWindow), 1, fp) == 1;
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

void FloatingDocumentLayoutModel_Destroy(FloatingDocumentLayoutModel* pModel)
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

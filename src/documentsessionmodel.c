#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "documentsessionmodel.h"

#define DOCSESSION_MAGIC 0x53534444u /* DDSS */
#define DOCSESSION_VERSION 1u

typedef struct DocumentSessionFileHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t count;
	int32_t activeIndex;
} DocumentSessionFileHeader;

BOOL DocumentSessionModel_SaveToFile(const DocumentSessionModel* pModel, PCWSTR pszFilePath)
{
	FILE* fp = NULL;
	DocumentSessionFileHeader header = { DOCSESSION_MAGIC, DOCSESSION_VERSION, 0, -1 };
	if (!pModel || !pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"wb") != 0 || !fp)
	{
		return FALSE;
	}

	header.count = (uint32_t)max(0, min(pModel->nEntryCount, (int)ARRAYSIZE(pModel->entries)));
	header.activeIndex = pModel->nActiveEntry;
	BOOL bOk = fwrite(&header, sizeof(header), 1, fp) == 1;
	if (bOk && header.count > 0)
	{
		bOk = fwrite(pModel->entries, sizeof(DocumentSessionEntry), header.count, fp) == header.count;
	}

	fclose(fp);
	return bOk;
}

BOOL DocumentSessionModel_LoadFromFile(PCWSTR pszFilePath, DocumentSessionModel* pModel)
{
	FILE* fp = NULL;
	DocumentSessionFileHeader header = { 0 };
	if (!pModel)
	{
		return FALSE;
	}

	memset(pModel, 0, sizeof(*pModel));
	pModel->nActiveEntry = -1;
	if (!pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"rb") != 0 || !fp)
	{
		return FALSE;
	}

	BOOL bOk = fread(&header, sizeof(header), 1, fp) == 1 &&
		header.magic == DOCSESSION_MAGIC &&
		header.version == DOCSESSION_VERSION &&
		header.count <= ARRAYSIZE(pModel->entries);
	if (bOk && header.count > 0)
	{
		bOk = fread(pModel->entries, sizeof(DocumentSessionEntry), header.count, fp) == header.count;
	}

	fclose(fp);
	if (bOk)
	{
		pModel->nEntryCount = (int)header.count;
		pModel->nActiveEntry = header.activeIndex;
	}

	return bOk;
}

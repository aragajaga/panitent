#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "dockfloatingmodel.h"

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
		bOk = fwrite(pModel->entries, sizeof(DockFloatingLayoutEntry), header.count, fp) == header.count;
	}

	fclose(fp);
	return bOk;
}

BOOL DockFloatingLayout_LoadFromFile(PCWSTR pszFilePath, DockFloatingLayoutFileModel* pModel)
{
	FILE* fp = NULL;
	DockFloatingFileHeader header = { 0 };
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
		header.magic == DOCKFLOATING_MAGIC &&
		header.version == DOCKFLOATING_VERSION &&
		header.count <= ARRAYSIZE(pModel->entries);
	if (bOk && header.count > 0)
	{
		bOk = fread(pModel->entries, sizeof(DockFloatingLayoutEntry), header.count, fp) == header.count;
	}

	fclose(fp);
	if (bOk)
	{
		pModel->nEntries = (int)header.count;
	}

	return bOk;
}

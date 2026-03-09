#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "windowlayoutcatalog.h"

#define WINDOW_LAYOUT_CATALOG_MAGIC 0x59414C57u /* WLAY */
#define WINDOW_LAYOUT_CATALOG_VERSION 1u

typedef struct WindowLayoutCatalogHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t count;
} WindowLayoutCatalogHeader;

static BOOL WindowLayoutCatalog_IsValidName(PCWSTR pszName)
{
	return pszName && pszName[0] != L'\0';
}

void WindowLayoutCatalog_Init(WindowLayoutCatalog* pCatalog)
{
	if (!pCatalog)
	{
		return;
	}

	memset(pCatalog, 0, sizeof(*pCatalog));
}

int WindowLayoutCatalog_FindByName(const WindowLayoutCatalog* pCatalog, PCWSTR pszName)
{
	if (!pCatalog || !WindowLayoutCatalog_IsValidName(pszName))
	{
		return -1;
	}

	for (int i = 0; i < pCatalog->nEntryCount; ++i)
	{
		if (_wcsicmp(pCatalog->entries[i].szName, pszName) == 0)
		{
			return i;
		}
	}

	return -1;
}

int WindowLayoutCatalog_FindById(const WindowLayoutCatalog* pCatalog, uint32_t uId)
{
	if (!pCatalog || uId == 0)
	{
		return -1;
	}

	for (int i = 0; i < pCatalog->nEntryCount; ++i)
	{
		if (pCatalog->entries[i].uId == uId)
		{
			return i;
		}
	}

	return -1;
}

BOOL WindowLayoutCatalog_Add(WindowLayoutCatalog* pCatalog, uint32_t uId, PCWSTR pszName)
{
	if (!pCatalog || uId == 0 || !WindowLayoutCatalog_IsValidName(pszName))
	{
		return FALSE;
	}

	if (pCatalog->nEntryCount >= WINDOW_LAYOUT_MAX_ENTRIES ||
		WindowLayoutCatalog_FindById(pCatalog, uId) >= 0 ||
		WindowLayoutCatalog_FindByName(pCatalog, pszName) >= 0)
	{
		return FALSE;
	}

	WindowLayoutCatalogEntry* pEntry = &pCatalog->entries[pCatalog->nEntryCount++];
	memset(pEntry, 0, sizeof(*pEntry));
	pEntry->uId = uId;
	wcscpy_s(pEntry->szName, ARRAYSIZE(pEntry->szName), pszName);
	return TRUE;
}

BOOL WindowLayoutCatalog_Rename(WindowLayoutCatalog* pCatalog, int index, PCWSTR pszName)
{
	if (!pCatalog || index < 0 || index >= pCatalog->nEntryCount || !WindowLayoutCatalog_IsValidName(pszName))
	{
		return FALSE;
	}

	int nExisting = WindowLayoutCatalog_FindByName(pCatalog, pszName);
	if (nExisting >= 0 && nExisting != index)
	{
		return FALSE;
	}

	wcscpy_s(pCatalog->entries[index].szName, ARRAYSIZE(pCatalog->entries[index].szName), pszName);
	return TRUE;
}

BOOL WindowLayoutCatalog_Delete(WindowLayoutCatalog* pCatalog, int index)
{
	if (!pCatalog || index < 0 || index >= pCatalog->nEntryCount)
	{
		return FALSE;
	}

	for (int i = index; i + 1 < pCatalog->nEntryCount; ++i)
	{
		pCatalog->entries[i] = pCatalog->entries[i + 1];
	}
	pCatalog->nEntryCount--;
	memset(&pCatalog->entries[pCatalog->nEntryCount], 0, sizeof(pCatalog->entries[pCatalog->nEntryCount]));
	return TRUE;
}

BOOL WindowLayoutCatalog_Move(WindowLayoutCatalog* pCatalog, int index, int indexTarget)
{
	if (!pCatalog ||
		index < 0 || index >= pCatalog->nEntryCount ||
		indexTarget < 0 || indexTarget >= pCatalog->nEntryCount ||
		index == indexTarget)
	{
		return FALSE;
	}

	WindowLayoutCatalogEntry temp = pCatalog->entries[index];
	if (index < indexTarget)
	{
		for (int i = index; i < indexTarget; ++i)
		{
			pCatalog->entries[i] = pCatalog->entries[i + 1];
		}
	}
	else {
		for (int i = index; i > indexTarget; --i)
		{
			pCatalog->entries[i] = pCatalog->entries[i - 1];
		}
	}
	pCatalog->entries[indexTarget] = temp;
	return TRUE;
}

uint32_t WindowLayoutCatalog_AllocateId(const WindowLayoutCatalog* pCatalog)
{
	uint32_t uNextId = 1;
	if (!pCatalog)
	{
		return uNextId;
	}

	for (int i = 0; i < pCatalog->nEntryCount; ++i)
	{
		if (pCatalog->entries[i].uId >= uNextId)
		{
			uNextId = pCatalog->entries[i].uId + 1;
		}
	}
	if (uNextId == 0)
	{
		uNextId = 1;
	}
	return uNextId;
}

BOOL WindowLayoutCatalog_LoadFromFile(PCWSTR pszFilePath, WindowLayoutCatalog* pCatalog, PersistLoadStatus* pStatus)
{
	FILE* fp = NULL;
	WindowLayoutCatalogHeader header = { 0 };

	if (pStatus)
	{
		*pStatus = PERSIST_LOAD_IO_ERROR;
	}
	if (!pCatalog)
	{
		return FALSE;
	}

	WindowLayoutCatalog_Init(pCatalog);
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
		return FALSE;
	}

	BOOL bOk =
		fread(&header, sizeof(header), 1, fp) == 1 &&
		header.magic == WINDOW_LAYOUT_CATALOG_MAGIC &&
		header.version == WINDOW_LAYOUT_CATALOG_VERSION &&
		header.count <= WINDOW_LAYOUT_MAX_ENTRIES;
	if (bOk && header.count > 0)
	{
		bOk = fread(pCatalog->entries, sizeof(WindowLayoutCatalogEntry), header.count, fp) == header.count;
	}
	fclose(fp);

	if (!bOk)
	{
		WindowLayoutCatalog_Init(pCatalog);
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_INVALID_FORMAT;
		}
		return FALSE;
	}

	pCatalog->nEntryCount = (int)header.count;
	if (pStatus)
	{
		*pStatus = PERSIST_LOAD_OK;
	}
	return TRUE;
}

BOOL WindowLayoutCatalog_SaveToFile(const WindowLayoutCatalog* pCatalog, PCWSTR pszFilePath)
{
	FILE* fp = NULL;
	WindowLayoutCatalogHeader header = { WINDOW_LAYOUT_CATALOG_MAGIC, WINDOW_LAYOUT_CATALOG_VERSION, 0 };

	if (!pCatalog || !pszFilePath || !pszFilePath[0] || pCatalog->nEntryCount < 0 || pCatalog->nEntryCount > WINDOW_LAYOUT_MAX_ENTRIES)
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"wb") != 0 || !fp)
	{
		return FALSE;
	}

	header.count = (uint32_t)pCatalog->nEntryCount;
	BOOL bOk = fwrite(&header, sizeof(header), 1, fp) == 1;
	if (bOk && header.count > 0)
	{
		bOk = fwrite(pCatalog->entries, sizeof(WindowLayoutCatalogEntry), header.count, fp) == header.count;
	}

	fclose(fp);
	return bOk;
}

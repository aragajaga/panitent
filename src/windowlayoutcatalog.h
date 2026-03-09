#pragma once

#include "precomp.h"
#include "persistload.h"

#define WINDOW_LAYOUT_MAX_ENTRIES 32
#define WINDOW_LAYOUT_NAME_MAX 64

typedef struct WindowLayoutCatalogEntry
{
	uint32_t uId;
	WCHAR szName[WINDOW_LAYOUT_NAME_MAX];
} WindowLayoutCatalogEntry;

typedef struct WindowLayoutCatalog
{
	int nEntryCount;
	WindowLayoutCatalogEntry entries[WINDOW_LAYOUT_MAX_ENTRIES];
} WindowLayoutCatalog;

void WindowLayoutCatalog_Init(WindowLayoutCatalog* pCatalog);
BOOL WindowLayoutCatalog_LoadFromFile(PCWSTR pszFilePath, WindowLayoutCatalog* pCatalog, PersistLoadStatus* pStatus);
BOOL WindowLayoutCatalog_SaveToFile(const WindowLayoutCatalog* pCatalog, PCWSTR pszFilePath);
int WindowLayoutCatalog_FindByName(const WindowLayoutCatalog* pCatalog, PCWSTR pszName);
int WindowLayoutCatalog_FindById(const WindowLayoutCatalog* pCatalog, uint32_t uId);
BOOL WindowLayoutCatalog_Add(WindowLayoutCatalog* pCatalog, uint32_t uId, PCWSTR pszName);
BOOL WindowLayoutCatalog_Rename(WindowLayoutCatalog* pCatalog, int index, PCWSTR pszName);
BOOL WindowLayoutCatalog_Delete(WindowLayoutCatalog* pCatalog, int index);
BOOL WindowLayoutCatalog_Move(WindowLayoutCatalog* pCatalog, int index, int indexTarget);
uint32_t WindowLayoutCatalog_AllocateId(const WindowLayoutCatalog* pCatalog);

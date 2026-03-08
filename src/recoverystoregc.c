#include "precomp.h"

#include "recoverystoregc.h"

#include "documentsessionmodel.h"
#include "floatingdocumentsessionmodel.h"
#include "recoverystorepersist.h"
#include "shell/pathutil.h"

static const ULONGLONG kRecoveryRetentionWindow100ns = 72ull * 60ull * 60ull * 10000000ull;

typedef struct RecoveryPathList
{
	LPWSTR* pItems;
	int nCount;
	int nCapacity;
} RecoveryPathList;

static BOOL RecoveryPathList_Push(RecoveryPathList* pList, PCWSTR pszPath)
{
	if (!pList || !pszPath || !pszPath[0])
	{
		return FALSE;
	}

	for (int i = 0; i < pList->nCount; ++i)
	{
		if (pList->pItems[i] && _wcsicmp(pList->pItems[i], pszPath) == 0)
		{
			return TRUE;
		}
	}

	if (pList->nCount == pList->nCapacity)
	{
		int nNewCapacity = pList->nCapacity > 0 ? pList->nCapacity * 2 : 32;
		LPWSTR* pNewItems = (LPWSTR*)realloc(pList->pItems, sizeof(LPWSTR) * nNewCapacity);
		if (!pNewItems)
		{
			return FALSE;
		}
		pList->pItems = pNewItems;
		pList->nCapacity = nNewCapacity;
	}

	size_t cch = wcslen(pszPath) + 1;
	LPWSTR pszCopy = (LPWSTR)malloc(sizeof(WCHAR) * cch);
	if (!pszCopy)
	{
		return FALSE;
	}

	wcscpy_s(pszCopy, cch, pszPath);
	pList->pItems[pList->nCount++] = pszCopy;
	return TRUE;
}

static void RecoveryPathList_Destroy(RecoveryPathList* pList)
{
	if (!pList)
	{
		return;
	}

	for (int i = 0; i < pList->nCount; ++i)
	{
		free(pList->pItems[i]);
	}

	free(pList->pItems);
	pList->pItems = NULL;
	pList->nCount = 0;
	pList->nCapacity = 0;
}

static void PanitentRecoveryStore_CollectDocumentSessionPaths(RecoveryPathList* pList, const DocumentSessionModel* pModel)
{
	if (!pList || !pModel)
	{
		return;
	}

	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		if (pModel->entries[i].nKind == DOCSESSION_ENTRY_RECOVERY)
		{
			RecoveryPathList_Push(pList, pModel->entries[i].szFilePath);
		}
	}
}

static void PanitentRecoveryStore_CollectFloatingDocumentSessionPaths(RecoveryPathList* pList, const FloatingDocumentSessionModel* pModel)
{
	if (!pList || !pModel)
	{
		return;
	}

	for (int i = 0; i < pModel->nEntryCount; ++i)
	{
		const FloatingDocumentSessionEntry* pEntry = &pModel->entries[i];
		for (int j = 0; j < pEntry->nWorkspaceCount; ++j)
		{
			const FloatingDocumentWorkspaceSession* pWorkspace = &pEntry->workspaces[j];
			for (int k = 0; k < pWorkspace->nFileCount; ++k)
			{
				if (pWorkspace->entries[k].nKind == DOCSESSION_ENTRY_RECOVERY)
				{
					RecoveryPathList_Push(pList, pWorkspace->entries[k].szFilePath);
				}
			}
		}
	}
}

static FILETIME PanitentRecoveryStore_GetUtcThreshold(void)
{
	FILETIME ftNow = { 0 };
	GetSystemTimeAsFileTime(&ftNow);
	ULARGE_INTEGER now = { 0 };
	now.LowPart = ftNow.dwLowDateTime;
	now.HighPart = ftNow.dwHighDateTime;
	ULONGLONG value = now.QuadPart;
	if (value > kRecoveryRetentionWindow100ns)
	{
		value -= kRecoveryRetentionWindow100ns;
	}
	else {
		value = 0;
	}

	FILETIME ftThreshold = { 0 };
	ULARGE_INTEGER threshold = { 0 };
	threshold.QuadPart = value;
	ftThreshold.dwLowDateTime = threshold.LowPart;
	ftThreshold.dwHighDateTime = threshold.HighPart;
	return ftThreshold;
}

BOOL PanitentRecoveryStore_RunStartupGc(void)
{
	RecoveryPathList keepPaths = { 0 };
	PTSTR pszDocumentSessionPath = NULL;
	PTSTR pszFloatingDocumentSessionPath = NULL;
	DocumentSessionModel* pDocumentSession = (DocumentSessionModel*)calloc(1, sizeof(DocumentSessionModel));
	FloatingDocumentSessionModel* pFloatingDocumentSession = (FloatingDocumentSessionModel*)calloc(1, sizeof(FloatingDocumentSessionModel));
	if (!pDocumentSession || !pFloatingDocumentSession)
	{
		free(pDocumentSession);
		free(pFloatingDocumentSession);
		return FALSE;
	}

	GetDocumentSessionFilePath(&pszDocumentSessionPath);
	if (pszDocumentSessionPath)
	{
		if (DocumentSessionModel_LoadFromFile(pszDocumentSessionPath, pDocumentSession))
		{
			PanitentRecoveryStore_CollectDocumentSessionPaths(&keepPaths, pDocumentSession);
		}
		free(pszDocumentSessionPath);
	}

	GetFloatingDocumentSessionFilePath(&pszFloatingDocumentSessionPath);
	if (pszFloatingDocumentSessionPath)
	{
		if (FloatingDocumentSessionModel_LoadFromFile(pszFloatingDocumentSessionPath, pFloatingDocumentSession))
		{
			PanitentRecoveryStore_CollectFloatingDocumentSessionPaths(&keepPaths, pFloatingDocumentSession);
		}
		free(pszFloatingDocumentSessionPath);
	}

	int nDeleted = 0;
	FILETIME utcThreshold = PanitentRecoveryStore_GetUtcThreshold();
	BOOL bResult = RecoveryStore_DeleteUnreferencedFilesOlderThanByPattern(
		L"recovery_*.pdr",
		(PCWSTR*)keepPaths.pItems,
		keepPaths.nCount,
		utcThreshold,
		&nDeleted);

	FloatingDocumentSessionModel_Destroy(pFloatingDocumentSession);
	RecoveryPathList_Destroy(&keepPaths);
	free(pDocumentSession);
	free(pFloatingDocumentSession);
	return bResult;
}

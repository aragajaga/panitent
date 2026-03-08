#include "precomp.h"

#include "recoverystoregc.h"

#include "documentsessionmodel.h"
#include "floatingdocumentsessionmodel.h"
#include "recoverystorepersist.h"
#include "shell/pathutil.h"

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

BOOL PanitentRecoveryStore_RunStartupGc(void)
{
	RecoveryPathList keepPaths = { 0 };
	PTSTR pszDocumentSessionPath = NULL;
	PTSTR pszFloatingDocumentSessionPath = NULL;
	DocumentSessionModel documentSession = { 0 };
	FloatingDocumentSessionModel floatingDocumentSession = { 0 };

	GetDocumentSessionFilePath(&pszDocumentSessionPath);
	if (pszDocumentSessionPath)
	{
		if (DocumentSessionModel_LoadFromFile(pszDocumentSessionPath, &documentSession))
		{
			PanitentRecoveryStore_CollectDocumentSessionPaths(&keepPaths, &documentSession);
		}
		free(pszDocumentSessionPath);
	}

	GetFloatingDocumentSessionFilePath(&pszFloatingDocumentSessionPath);
	if (pszFloatingDocumentSessionPath)
	{
		if (FloatingDocumentSessionModel_LoadFromFile(pszFloatingDocumentSessionPath, &floatingDocumentSession))
		{
			PanitentRecoveryStore_CollectFloatingDocumentSessionPaths(&keepPaths, &floatingDocumentSession);
		}
		free(pszFloatingDocumentSessionPath);
	}

	int nDeleted = 0;
	BOOL bResult = RecoveryStore_DeleteUnreferencedFilesByPattern(
		L"recovery_*.pdr",
		(PCWSTR*)keepPaths.pItems,
		keepPaths.nCount,
		&nDeleted);

	FloatingDocumentSessionModel_Destroy(&floatingDocumentSession);
	RecoveryPathList_Destroy(&keepPaths);
	return bResult;
}

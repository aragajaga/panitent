#include "precomp.h"

#include "recoverystorepersist.h"

#include "recoverystore.h"
#include "shell/pathutil.h"

BOOL RecoveryStore_DeleteFilesByPattern(PCWSTR pszPattern, int* pnDeleted)
{
	PTSTR pszAppData = NULL;
	GetAppDataFilePath(NULL, &pszAppData);
	if (!pszAppData)
	{
		return FALSE;
	}

	BOOL bResult = RecoveryStore_DeleteFilesInDirectory(pszAppData, pszPattern, pnDeleted);
	free(pszAppData);
	return bResult;
}

BOOL RecoveryStore_DeleteUnreferencedFilesByPattern(
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	int* pnDeleted)
{
	PTSTR pszAppData = NULL;
	GetAppDataFilePath(NULL, &pszAppData);
	if (!pszAppData)
	{
		return FALSE;
	}

	BOOL bResult = RecoveryStore_DeleteUnreferencedFilesInDirectory(
		pszAppData,
		pszPattern,
		pKeepPaths,
		cKeepPaths,
		pnDeleted);
	free(pszAppData);
	return bResult;
}

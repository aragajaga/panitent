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

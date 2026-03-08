#include "precomp.h"

#include <strsafe.h>

#include "recoverystore.h"

BOOL RecoveryStore_DeleteFilesInDirectory(PCWSTR pszDirectory, PCWSTR pszPattern, int* pnDeleted)
{
	if (pnDeleted)
	{
		*pnDeleted = 0;
	}

	if (!pszDirectory || !pszDirectory[0] || !pszPattern || !pszPattern[0])
	{
		return FALSE;
	}

	WCHAR szSearchPath[MAX_PATH] = L"";
	if (FAILED(StringCchPrintfW(szSearchPath, ARRAYSIZE(szSearchPath), L"%s\\%s", pszDirectory, pszPattern)))
	{
		return FALSE;
	}

	WIN32_FIND_DATAW findData = { 0 };
	HANDLE hFind = FindFirstFileW(szSearchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		return dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND;
	}

	BOOL bOk = TRUE;
	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		WCHAR szFilePath[MAX_PATH] = L"";
		if (FAILED(StringCchPrintfW(szFilePath, ARRAYSIZE(szFilePath), L"%s\\%s", pszDirectory, findData.cFileName)))
		{
			bOk = FALSE;
			break;
		}

		if (!DeleteFileW(szFilePath))
		{
			bOk = FALSE;
			break;
		}

		if (pnDeleted)
		{
			(*pnDeleted)++;
		}
	} while (FindNextFileW(hFind, &findData));

	DWORD dwLastError = GetLastError();
	FindClose(hFind);
	if (dwLastError != ERROR_NO_MORE_FILES)
	{
		return FALSE;
	}

	return bOk;
}

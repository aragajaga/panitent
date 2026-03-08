#include "precomp.h"

#include <strsafe.h>

#include "recoverystore.h"

static BOOL RecoveryStore_PathInKeepList(PCWSTR pszFilePath, PCWSTR* pKeepPaths, int cKeepPaths)
{
	if (!pszFilePath || !pKeepPaths || cKeepPaths <= 0)
	{
		return FALSE;
	}

	for (int i = 0; i < cKeepPaths; ++i)
	{
		if (pKeepPaths[i] && _wcsicmp(pKeepPaths[i], pszFilePath) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static BOOL RecoveryStore_IsOlderThanThreshold(const WIN32_FIND_DATAW* pFindData, FILETIME utcThreshold)
{
	if (!pFindData)
	{
		return FALSE;
	}

	return CompareFileTime(&pFindData->ftLastWriteTime, &utcThreshold) < 0;
}

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

BOOL RecoveryStore_DeleteUnreferencedFilesInDirectory(
	PCWSTR pszDirectory,
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	int* pnDeleted)
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

		if (RecoveryStore_PathInKeepList(szFilePath, pKeepPaths, cKeepPaths))
		{
			continue;
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

BOOL RecoveryStore_DeleteUnreferencedFilesOlderThanInDirectory(
	PCWSTR pszDirectory,
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	FILETIME utcThreshold,
	int* pnDeleted)
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

		if (RecoveryStore_PathInKeepList(szFilePath, pKeepPaths, cKeepPaths) ||
			!RecoveryStore_IsOlderThanThreshold(&findData, utcThreshold))
		{
			continue;
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

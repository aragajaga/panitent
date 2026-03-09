#include "precomp.h"

#include <strsafe.h>

#include "persistfile.h"

BOOL PersistFile_QuarantineInvalid(PCWSTR pszFilePath)
{
	if (!pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	DWORD dwAttrs = GetFileAttributesW(pszFilePath);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES)
	{
		return FALSE;
	}

	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);

	WCHAR szBackupPath[MAX_PATH] = L"";
	if (FAILED(StringCchPrintfW(
		szBackupPath,
		ARRAYSIZE(szBackupPath),
		L"%s.invalid.%04u%02u%02u_%02u%02u%02u.bak",
		pszFilePath,
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond)))
	{
		return FALSE;
	}

	if (MoveFileExW(pszFilePath, szBackupPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
	{
		return TRUE;
	}

	return DeleteFileW(pszFilePath);
}

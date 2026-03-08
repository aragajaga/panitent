#pragma once

#include "precomp.h"

BOOL RecoveryStore_DeleteFilesByPattern(PCWSTR pszPattern, int* pnDeleted);
BOOL RecoveryStore_DeleteUnreferencedFilesByPattern(
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	int* pnDeleted);
BOOL RecoveryStore_DeleteUnreferencedFilesOlderThanByPattern(
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	FILETIME utcThreshold,
	int* pnDeleted);

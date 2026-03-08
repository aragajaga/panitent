#pragma once

#include "precomp.h"

BOOL RecoveryStore_DeleteFilesInDirectory(PCWSTR pszDirectory, PCWSTR pszPattern, int* pnDeleted);
BOOL RecoveryStore_DeleteUnreferencedFilesInDirectory(
	PCWSTR pszDirectory,
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	int* pnDeleted);
BOOL RecoveryStore_DeleteUnreferencedFilesOlderThanInDirectory(
	PCWSTR pszDirectory,
	PCWSTR pszPattern,
	PCWSTR* pKeepPaths,
	int cKeepPaths,
	FILETIME utcThreshold,
	int* pnDeleted);

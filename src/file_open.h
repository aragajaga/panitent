#ifndef FILE_OPEN_H
#define FILE_OPEN_H

#include "stdafx.h"

#include <initguid.h>
#include <shlwapi.h>

#define INDEX_PNG 1

void FileOpenPng(LPWSTR pszPath);
int FileOpen();
int FileSave();

#endif /* FILE_OPEN_H */

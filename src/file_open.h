#ifndef PANITENT_FILE_OPEN_H
#define PANITENT_FILE_OPEN_H

#include "precomp.h"

#include <initguid.h>
#include <shlwapi.h>

#define INDEX_PNG 1

void FileOpenPng(LPWSTR pszPath);
int FileOpen();
int FileSave();

#endif /* PANITENT_FILE_OPEN_H */

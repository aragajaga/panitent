#ifndef PANITENT_FILE_OPEN_H
#define PANITENT_FILE_OPEN_H

#include "crefptr.h"

#define INDEX_PNG 1

void FileOpenPng(LPWSTR pszPath);
BOOL init_open_file_dialog(LPWSTR *pszPath);
BOOL init_save_file_dialog(LPWSTR *pszPath);

#endif /* PANITENT_FILE_OPEN_H */

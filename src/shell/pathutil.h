#pragma once

#ifndef PANITENT_SHELL_PATHUTIL_H_
#define PANITENT_SHELL_PATHUTIL_H_

void InitAppData(LPWSTR* ppszAppData);
void GetAppDataFilePath(LPCWSTR pszFile, LPWSTR* pszResult);
void GetModulePath();
void GetWorkDir();
void GetSettingsFilePath(PTSTR* ppszSettingsFilePath);

#endif  /* PANITENT_SHELL_PATHUTIL_H_ */

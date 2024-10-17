#include "../precomp.h"

#include <tchar.h>
#include "pathutil.h"

static const WCHAR g_kAppData[] = L"\\Aragajaga\\Panit.ent";

void InitAppData(LPWSTR* ppszAppData)
{
    PWSTR pszShAppData = NULL;
    /* Get %ROAMINGAPPDATA% path */
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &pszShAppData);
    if (FAILED(hr))
    {
        /* On error, COM not initialized string memry. Freely return as is */
        return;
    }

    /* Prepare Panit.ent AppData path */
    PWSTR pszAppData = NULL;
    size_t shAppDataLen = wcslen(pszShAppData);
    /* Break if shell AppData string is empty */
    if (!shAppDataLen)
    {
        goto error;
    }

    /* g_kAppData is already NULL-terminated. So ARRAYSIZE will evaluate as strlen + 1 */
    pszAppData = (PWSTR)malloc((shAppDataLen + ARRAYSIZE(g_kAppData)) * sizeof(WCHAR));
    if (!pszAppData)
    {
        /* Memory allocation failed. Free pszShAppData and return */
        goto error;
    }

    /* Prepare Panit.ent AppData path with %ROAMINGAPPDATA% */
    wcscpy_s(pszAppData, MAX_PATH, pszShAppData);
    /* Append application site directory */
    PathAppend(pszAppData, g_kAppData);

    /* Create directory if it not exists */
    SHCreateDirectoryEx(NULL, pszAppData, NULL);

    /* Return evaluated path */
    *ppszAppData = pszAppData;

    /* SH AppData no more needed */
    CoTaskMemFree(pszShAppData);
    return;

error:
    if (pszShAppData)
    {
        CoTaskMemFree(pszShAppData);
    }

    if (pszAppData)
    {
        free(pszAppData);
    }
}

void GetAppDataFilePath(LPCWSTR pszFile, LPWSTR* pszResult)
{
    LPWSTR pszAppData = NULL;
    InitAppData(&pszAppData);

    if (!pszAppData)
    {
        goto error;
    }

    LPWSTR pszFilePath = NULL;
    size_t appDataLen = _tcslen(pszAppData);
    pszFilePath = (LPWSTR)malloc((appDataLen + _tcslen(pszFile)) * sizeof(WCHAR));
    if (!pszFilePath)
    {
        goto error;
    }

    wcscpy_s(pszFilePath, wcslen(pszAppData), pszAppData);
    free(pszAppData);
    PathAppend(pszFilePath, pszFile);

    *pszResult = pszFilePath;
    return;

error:
    if (pszAppData)
    {
        free(pszAppData);
    }
}

void GetModulePath()
{
    HMODULE hModule = GetModuleHandle(NULL);

    DWORD dwLen = GetModuleFileName(hModule, NULL, 0);
    if (dwLen)
    {
        PWSTR pszModulePath = (PWSTR)malloc((dwLen + 1) * sizeof(WCHAR));
        if (pszModulePath)
        {
            GetModuleFileName(hModule, pszModulePath, dwLen + 1);
        }
    }
}

void GetWorkDir()
{
    DWORD dwLen = GetCurrentDirectory(0, NULL);
    if (dwLen)
    {
        PWSTR pszWorkDir = (PWSTR)malloc((dwLen + 1) * sizeof(WCHAR));
        if (pszWorkDir)
        {
            GetCurrentDirectoryW(dwLen + 1, pszWorkDir);
        }
    }
}

void GetSettingsFilePath(PTSTR* ppszSettingsFilePath)
{
    LPWSTR pszSettingsFilePath = NULL;
    GetAppDataFilePath(_T("\\settings.dat"), &pszSettingsFilePath);

    if (!pszSettingsFilePath)
    {
        return;
    }

    *ppszSettingsFilePath = pszSettingsFilePath;
}

#include "../precomp.h"

#include <tchar.h>
#include "pathutil.h"

static const WCHAR g_kAppData[] = L"Aragajaga\\Panit.ent";

void InitAppData(LPWSTR* ppszAppData)
{
    if (!ppszAppData)
    {
        return;
    }

    *ppszAppData = NULL;

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
    size_t appDataSuffixLen = wcslen(g_kAppData);
    /* Break if shell AppData string is empty */
    if (!shAppDataLen)
    {
        goto error;
    }

    pszAppData = (PWSTR)calloc(shAppDataLen + 1 + appDataSuffixLen + 1, sizeof(WCHAR));
    if (!pszAppData)
    {
        /* Memory allocation failed. Free pszShAppData and return */
        goto error;
    }

    if (FAILED(StringCchPrintfW(
        pszAppData,
        shAppDataLen + 1 + appDataSuffixLen + 1,
        L"%s\\%s",
        pszShAppData,
        g_kAppData)))
    {
        goto error;
    }

    /* Create directory if it not exists */
    SHCreateDirectoryExW(NULL, pszAppData, NULL);

    /* Return evaluated path */
    *ppszAppData = pszAppData;
    pszAppData = NULL;

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
    if (!pszResult)
    {
        return;
    }

    *pszResult = NULL;

    LPWSTR pszAppData = NULL;
    InitAppData(&pszAppData);

    if (!pszAppData)
    {
        goto error;
    }

    if (!pszFile || pszFile[0] == L'\0')
    {
        *pszResult = pszAppData;
        return;
    }

    while (*pszFile == L'\\' || *pszFile == L'/')
    {
        ++pszFile;
    }

    LPWSTR pszFilePath = NULL;
    size_t appDataLen = wcslen(pszAppData);
    size_t fileLen = wcslen(pszFile);
    pszFilePath = (LPWSTR)calloc(appDataLen + 1 + fileLen + 1, sizeof(WCHAR));
    if (!pszFilePath)
    {
        goto error;
    }

    if (FAILED(StringCchPrintfW(
        pszFilePath,
        appDataLen + 1 + fileLen + 1,
        L"%s\\%s",
        pszAppData,
        pszFile)))
    {
        free(pszFilePath);
        goto error;
    }

    free(pszAppData);
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
    GetAppDataFilePath(_T("settings.dat"), &pszSettingsFilePath);

    if (!pszSettingsFilePath)
    {
        return;
    }

    *ppszSettingsFilePath = pszSettingsFilePath;
}

void GetDockLayoutFilePath(PTSTR* ppszDockLayoutFilePath)
{
    LPWSTR pszDockLayoutFilePath = NULL;
    GetAppDataFilePath(_T("docklayout.dat"), &pszDockLayoutFilePath);

    if (!pszDockLayoutFilePath)
    {
        return;
    }

    *ppszDockLayoutFilePath = pszDockLayoutFilePath;
}

void GetDockFloatingFilePath(PTSTR* ppszDockFloatingFilePath)
{
    LPWSTR pszDockFloatingFilePath = NULL;
    GetAppDataFilePath(_T("dockfloating.dat"), &pszDockFloatingFilePath);

    if (!pszDockFloatingFilePath)
    {
        return;
    }

    *ppszDockFloatingFilePath = pszDockFloatingFilePath;
}

void GetDocumentSessionFilePath(PTSTR* ppszDocumentSessionFilePath)
{
    LPWSTR pszDocumentSessionFilePath = NULL;
    GetAppDataFilePath(_T("documentsession.dat"), &pszDocumentSessionFilePath);

    if (!pszDocumentSessionFilePath)
    {
        return;
    }

    *ppszDocumentSessionFilePath = pszDocumentSessionFilePath;
}

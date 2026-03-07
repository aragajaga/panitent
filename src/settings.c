#include "precomp.h"

#include "settings.h"

#include <stddef.h>
#include <tchar.h>

#include "shell/pathutil.h"

void Panitent_DefaultSettings(PNTSETTINGS* pSettings)
{
    if (!pSettings)
    {
        return;
    }

    memset(pSettings, 0, sizeof(*pSettings));
    pSettings->x = CW_USEDEFAULT;
    pSettings->y = CW_USEDEFAULT;
    pSettings->width = 1280;
    pSettings->height = 800;
    pSettings->bRememberWindowPos = FALSE;
    pSettings->bLegacyFileDialogs = FALSE;
    pSettings->bEnablePenTablet = FALSE;
    pSettings->iToolbarIconTheme = 0;
    pSettings->bUseStandardWindowFrame = TRUE;
}

BOOL Panitent_ReadSettings(PNTSETTINGS* pSettings)
{
    if (!pSettings)
    {
        return FALSE;
    }

    PTSTR pszSettingsFilePath = NULL;
    GetSettingsFilePath(&pszSettingsFilePath);
    if (!pszSettingsFilePath)
    {
        return FALSE;
    }

    FILE* fp = NULL;
    errno_t result = _tfopen_s(&fp, pszSettingsFilePath, _T("rb"));
    free(pszSettingsFilePath);
    if (result != 0 || !fp)
    {
        return FALSE;
    }

    size_t nRead = fread(pSettings, 1, sizeof(PNTSETTINGS), fp);
    fclose(fp);
    return nRead >= offsetof(PNTSETTINGS, bUseStandardWindowFrame);
}

BOOL Panitent_WriteSettings(const PNTSETTINGS* pSettings)
{
    if (!pSettings)
    {
        return FALSE;
    }

    PTSTR pszSettingsFilePath = NULL;
    GetSettingsFilePath(&pszSettingsFilePath);
    if (!pszSettingsFilePath)
    {
        return FALSE;
    }

    FILE* fp = NULL;
    errno_t result = _tfopen_s(&fp, pszSettingsFilePath, _T("wb"));
    free(pszSettingsFilePath);
    if (result != 0 || !fp)
    {
        return FALSE;
    }

    size_t nWritten = fwrite(pSettings, 1, sizeof(PNTSETTINGS), fp);
    fclose(fp);
    return nWritten == sizeof(PNTSETTINGS);
}

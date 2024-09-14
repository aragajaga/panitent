#include "precomp.h"

#include "settings.h"

#include <tchar.h>

void Panitent_ReadSettings(PNTSETTINGS* pSettings)
{
    PTSTR pszSettingsFilePath = NULL;
    GetSettingsFilePath(&pszSettingsFilePath);
    if (!pszSettingsFilePath)
    {
        return;
    }

    FILE* fp;
    errno_t result = _tfopen_s(&fp, pszSettingsFilePath, _T("rb"));
    free(pszSettingsFilePath);
    assert(result);

    if (!result || !fp)
    {
        return;
    }

    fread(pSettings, sizeof(PNTSETTINGS), 1, fp);
    fclose(fp);
}

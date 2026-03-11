#include "precomp.h"

#include "windowlayoutprofile.h"

#include "dockfloatingmodel.h"
#include "dockmodelio.h"
#include "floatingdocumentlayoutmodel.h"
#include "persistfile.h"
#include "persistload.h"
#include "shell/pathutil.h"

static BOOL WindowLayoutProfile_GetPath(uint32_t uId, PCWSTR pszSuffix, PTSTR* ppszPath)
{
    WCHAR szRelative[MAX_PATH] = L"";
    if (!ppszPath || !pszSuffix || !pszSuffix[0] || uId == 0)
    {
        return FALSE;
    }

    StringCchPrintfW(szRelative, ARRAYSIZE(szRelative), L"windowlayout_%08X_%ls", uId, pszSuffix);
    GetAppDataFilePath(szRelative, ppszPath);
    return *ppszPath != NULL;
}

BOOL WindowLayoutProfile_GetDockLayoutPath(uint32_t uId, PTSTR* ppszPath)
{
    return WindowLayoutProfile_GetPath(uId, L"docklayout.dat", ppszPath);
}

BOOL WindowLayoutProfile_GetDockFloatingPath(uint32_t uId, PTSTR* ppszPath)
{
    return WindowLayoutProfile_GetPath(uId, L"dockfloating.dat", ppszPath);
}

BOOL WindowLayoutProfile_GetFloatDocLayoutPath(uint32_t uId, PTSTR* ppszPath)
{
    return WindowLayoutProfile_GetPath(uId, L"floatdoclayout.dat", ppszPath);
}

BOOL WindowLayoutProfile_SaveBundle(
    uint32_t uId,
    const DockModelNode* pDockLayoutModel,
    const DockFloatingLayoutFileModel* pDockFloatingModel,
    const FloatingDocumentLayoutModel* pFloatDocLayoutModel)
{
    PTSTR pszDockLayoutPath = NULL;
    PTSTR pszDockFloatingPath = NULL;
    PTSTR pszFloatDocLayoutPath = NULL;
    BOOL bSaved = FALSE;

    if (uId == 0 || !pDockLayoutModel || !pDockFloatingModel || !pFloatDocLayoutModel)
    {
        return FALSE;
    }

    if (!WindowLayoutProfile_GetDockLayoutPath(uId, &pszDockLayoutPath) ||
        !WindowLayoutProfile_GetDockFloatingPath(uId, &pszDockFloatingPath) ||
        !WindowLayoutProfile_GetFloatDocLayoutPath(uId, &pszFloatDocLayoutPath))
    {
        goto cleanup;
    }

    bSaved =
        DockModelIO_SaveToFile(pDockLayoutModel, pszDockLayoutPath) &&
        DockFloatingLayout_SaveToFile(pDockFloatingModel, pszDockFloatingPath) &&
        FloatingDocumentLayoutModel_SaveToFile(pFloatDocLayoutModel, pszFloatDocLayoutPath);

cleanup:
    free(pszDockLayoutPath);
    free(pszDockFloatingPath);
    free(pszFloatDocLayoutPath);
    return bSaved;
}

BOOL WindowLayoutProfile_LoadBundle(
    uint32_t uId,
    DockModelNode** ppDockLayoutModel,
    DockFloatingLayoutFileModel* pDockFloatingModel,
    FloatingDocumentLayoutModel* pFloatDocLayoutModel)
{
    PTSTR pszDockLayoutPath = NULL;
    PTSTR pszDockFloatingPath = NULL;
    PTSTR pszFloatDocLayoutPath = NULL;
    DockModelNode* pDockLayoutModel = NULL;
    PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
    BOOL bLoaded = FALSE;

    if (!ppDockLayoutModel || !pDockFloatingModel || !pFloatDocLayoutModel || uId == 0)
    {
        return FALSE;
    }

    *ppDockLayoutModel = NULL;
    memset(pDockFloatingModel, 0, sizeof(*pDockFloatingModel));
    memset(pFloatDocLayoutModel, 0, sizeof(*pFloatDocLayoutModel));

    if (!WindowLayoutProfile_GetDockLayoutPath(uId, &pszDockLayoutPath) ||
        !WindowLayoutProfile_GetDockFloatingPath(uId, &pszDockFloatingPath) ||
        !WindowLayoutProfile_GetFloatDocLayoutPath(uId, &pszFloatDocLayoutPath))
    {
        goto cleanup;
    }

    pDockLayoutModel = DockModelIO_LoadFromFileEx(pszDockLayoutPath, &loadStatus);
    if (!pDockLayoutModel)
    {
        if (loadStatus == PERSIST_LOAD_INVALID_FORMAT)
        {
            PersistFile_QuarantineInvalid(pszDockLayoutPath);
        }
        goto cleanup;
    }

    loadStatus = PERSIST_LOAD_IO_ERROR;
    if (!DockFloatingLayout_LoadFromFileEx(pszDockFloatingPath, pDockFloatingModel, &loadStatus))
    {
        if (loadStatus == PERSIST_LOAD_INVALID_FORMAT)
        {
            PersistFile_QuarantineInvalid(pszDockFloatingPath);
        }
        goto cleanup;
    }

    loadStatus = PERSIST_LOAD_IO_ERROR;
    if (!FloatingDocumentLayoutModel_LoadFromFileEx(pszFloatDocLayoutPath, pFloatDocLayoutModel, &loadStatus))
    {
        if (loadStatus == PERSIST_LOAD_INVALID_FORMAT)
        {
            PersistFile_QuarantineInvalid(pszFloatDocLayoutPath);
        }
        goto cleanup;
    }

    *ppDockLayoutModel = pDockLayoutModel;
    pDockLayoutModel = NULL;
    bLoaded = TRUE;

cleanup:
    if (pDockLayoutModel)
    {
        DockModel_Destroy(pDockLayoutModel);
    }
    if (!bLoaded)
    {
        DockFloatingLayout_Destroy(pDockFloatingModel);
        FloatingDocumentLayoutModel_Destroy(pFloatDocLayoutModel);
    }
    free(pszDockLayoutPath);
    free(pszDockFloatingPath);
    free(pszFloatDocLayoutPath);
    return bLoaded;
}

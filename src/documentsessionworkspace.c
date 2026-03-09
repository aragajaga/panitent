#include "precomp.h"

#include "documentsessionworkspace.h"

#include "document.h"
#include "documentrecovery.h"
#include "viewport.h"
#include "workspacecontainer.h"

static BOOL DocumentSessionWorkspace_IsFileBackedViewport(ViewportWindow* pViewportWindow)
{
    Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
    return pDocument &&
        Document_IsFilePathSet(pDocument) &&
        Document_GetFilePath(pDocument) &&
        Document_GetFilePath(pDocument)[0] != L'\0';
}

BOOL DocumentSessionWorkspace_CaptureEntries(
    WorkspaceContainer* pWorkspaceContainer,
    DocumentSessionEntry* pEntries,
    int cEntries,
    int* pnEntryCount,
    int* pnActiveEntry,
    FnDocumentSessionWorkspaceBuildRecoveryPath pfnBuildRecoveryPath,
    void* pBuildRecoveryPathUserData)
{
    if (pnEntryCount)
    {
        *pnEntryCount = 0;
    }
    if (pnActiveEntry)
    {
        *pnActiveEntry = -1;
    }

    if (!pWorkspaceContainer || !pEntries || cEntries <= 0)
    {
        return FALSE;
    }

    ViewportWindow* pActiveViewport = WorkspaceContainer_GetCurrentViewport(pWorkspaceContainer);
    int nEntryCount = 0;
    int nActiveEntry = -1;

    for (int i = 0; i < WorkspaceContainer_GetViewportCount(pWorkspaceContainer) && nEntryCount < cEntries; ++i)
    {
        ViewportWindow* pViewportWindow = WorkspaceContainer_GetViewportAt(pWorkspaceContainer, i);
        Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
        if (!pDocument)
        {
            continue;
        }

        DocumentSessionEntry* pEntry = &pEntries[nEntryCount];
        memset(pEntry, 0, sizeof(*pEntry));

        if (DocumentSessionWorkspace_IsFileBackedViewport(pViewportWindow) && !Document_IsDirty(pDocument))
        {
            pEntry->nKind = DOCSESSION_ENTRY_FILE;
            wcscpy_s(pEntry->szFilePath, ARRAYSIZE(pEntry->szFilePath), Document_GetFilePath(pDocument));
        }
        else {
            pEntry->nKind = DOCSESSION_ENTRY_RECOVERY;
            if (!pfnBuildRecoveryPath ||
                !pfnBuildRecoveryPath(
                    pBuildRecoveryPathUserData,
                    nEntryCount,
                    pEntry->szFilePath,
                    ARRAYSIZE(pEntry->szFilePath)) ||
                !DocumentRecovery_Save(pDocument, pEntry->szFilePath))
            {
                continue;
            }
        }

        if (pViewportWindow == pActiveViewport)
        {
            nActiveEntry = nEntryCount;
        }

        nEntryCount++;
    }

    if (pnEntryCount)
    {
        *pnEntryCount = nEntryCount;
    }
    if (pnActiveEntry)
    {
        *pnActiveEntry = nActiveEntry;
    }
    return TRUE;
}

BOOL DocumentSessionWorkspace_RestoreEntries(
    WorkspaceContainer* pWorkspaceContainer,
    const DocumentSessionEntry* pEntries,
    int nEntryCount,
    int nActiveEntry)
{
    if (!pWorkspaceContainer || !pEntries || nEntryCount < 0)
    {
        return FALSE;
    }

    BOOL bRestoredAny = FALSE;

    for (int i = 0; i < nEntryCount; ++i)
    {
        if (i == nActiveEntry)
        {
            continue;
        }

        BOOL bOpened =
            pEntries[i].nKind == DOCSESSION_ENTRY_FILE ?
            Document_OpenFileInWorkspace(pEntries[i].szFilePath, pWorkspaceContainer) :
            DocumentRecovery_OpenInWorkspace(pEntries[i].szFilePath, pWorkspaceContainer);
        if (bOpened)
        {
            bRestoredAny = TRUE;
        }
    }

    if (nActiveEntry >= 0 && nActiveEntry < nEntryCount)
    {
        BOOL bOpened =
            pEntries[nActiveEntry].nKind == DOCSESSION_ENTRY_FILE ?
            Document_OpenFileInWorkspace(pEntries[nActiveEntry].szFilePath, pWorkspaceContainer) :
            DocumentRecovery_OpenInWorkspace(pEntries[nActiveEntry].szFilePath, pWorkspaceContainer);
        if (bOpened)
        {
            bRestoredAny = TRUE;
        }
    }

    return bRestoredAny;
}

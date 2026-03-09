#pragma once

#include "documentsessionmodel.h"

typedef struct WorkspaceContainer WorkspaceContainer;

typedef BOOL (*FnDocumentSessionWorkspaceBuildRecoveryPath)(
    void* pUserData,
    int nIndex,
    WCHAR* pszBuffer,
    size_t cchBuffer);

BOOL DocumentSessionWorkspace_CaptureEntries(
    WorkspaceContainer* pWorkspaceContainer,
    DocumentSessionEntry* pEntries,
    int cEntries,
    int* pnEntryCount,
    int* pnActiveEntry,
    FnDocumentSessionWorkspaceBuildRecoveryPath pfnBuildRecoveryPath,
    void* pBuildRecoveryPathUserData);

BOOL DocumentSessionWorkspace_RestoreEntries(
    WorkspaceContainer* pWorkspaceContainer,
    const DocumentSessionEntry* pEntries,
    int nEntryCount,
    int nActiveEntry);

#pragma once

#include "precomp.h"

typedef struct _Document Document;
typedef struct WorkspaceContainer WorkspaceContainer;

BOOL DocumentRecovery_Save(Document* pDocument, PCWSTR pszPath);
Document* DocumentRecovery_Load(PCWSTR pszPath);
BOOL DocumentRecovery_OpenInWorkspace(PCWSTR pszPath, WorkspaceContainer* pWorkspaceContainer);

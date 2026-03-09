#include "precomp.h"

#include "documentsessionpersist.h"

#include "document.h"
#include "documentrecovery.h"
#include "documentsessionmodel.h"
#include "panitentapp.h"
#include "persistfile.h"
#include "recoverystorepersist.h"
#include "viewport.h"
#include "workspacecontainer.h"
#include "shell/pathutil.h"

static BOOL DocumentSessionPersist_IsFileBackedViewport(ViewportWindow* pViewportWindow)
{
	Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
	return pDocument && Document_IsFilePathSet(pDocument) && Document_GetFilePath(pDocument) && Document_GetFilePath(pDocument)[0] != L'\0';
}

static BOOL DocumentSessionPersist_BuildRecoveryPath(WCHAR* pszBuffer, size_t cchBuffer, int index)
{
	if (!pszBuffer || cchBuffer == 0)
	{
		return FALSE;
	}

	WCHAR szRelative[MAX_PATH] = L"";
	StringCchPrintfW(szRelative, ARRAYSIZE(szRelative), L"recovery_main_%02d.pdr", index);
	PTSTR pszFilePath = NULL;
	GetAppDataFilePath(szRelative, &pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	StringCchCopyW(pszBuffer, cchBuffer, pszFilePath);
	free(pszFilePath);
	return TRUE;
}

BOOL PanitentDocumentSession_Save(PanitentApp* pPanitentApp)
{
	WorkspaceContainer* pWorkspaceContainer;
	DocumentSessionModel model = { 0 };
	PTSTR pszFilePath = NULL;

	if (!pPanitentApp)
	{
		return FALSE;
	}

	pWorkspaceContainer = PanitentApp_GetWorkspaceContainer(pPanitentApp);
	if (!pWorkspaceContainer)
	{
		return FALSE;
	}

	RecoveryStore_DeleteFilesByPattern(L"recovery_main_*.pdr", NULL);

	ViewportWindow* pActiveViewport = WorkspaceContainer_GetCurrentViewport(pWorkspaceContainer);
	for (int i = 0; i < WorkspaceContainer_GetViewportCount(pWorkspaceContainer) && model.nEntryCount < ARRAYSIZE(model.entries); ++i)
	{
		ViewportWindow* pViewportWindow = WorkspaceContainer_GetViewportAt(pWorkspaceContainer, i);
		Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
		if (!pDocument)
		{
			continue;
		}

		if (DocumentSessionPersist_IsFileBackedViewport(pViewportWindow) && !Document_IsDirty(pDocument))
		{
			model.entries[model.nEntryCount].nKind = DOCSESSION_ENTRY_FILE;
			wcscpy_s(
				model.entries[model.nEntryCount].szFilePath,
				ARRAYSIZE(model.entries[model.nEntryCount].szFilePath),
				Document_GetFilePath(pDocument));
		}
		else {
			model.entries[model.nEntryCount].nKind = DOCSESSION_ENTRY_RECOVERY;
			if (!DocumentSessionPersist_BuildRecoveryPath(
				model.entries[model.nEntryCount].szFilePath,
				ARRAYSIZE(model.entries[model.nEntryCount].szFilePath),
				model.nEntryCount) ||
				!DocumentRecovery_Save(pDocument, model.entries[model.nEntryCount].szFilePath))
			{
				continue;
			}
		}
		if (pViewportWindow == pActiveViewport)
		{
			model.nActiveEntry = model.nEntryCount;
		}
		model.nEntryCount++;
	}

	GetDocumentSessionFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	BOOL bResult = DocumentSessionModel_SaveToFile(&model, pszFilePath);
	free(pszFilePath);
	return bResult;
}

BOOL PanitentDocumentSession_Restore(PanitentApp* pPanitentApp)
{
	DocumentSessionModel model = { 0 };
	PTSTR pszFilePath = NULL;
	WorkspaceContainer* pWorkspaceContainer;

	if (!pPanitentApp)
	{
		return FALSE;
	}

	pWorkspaceContainer = PanitentApp_GetWorkspaceContainer(pPanitentApp);
	if (!pWorkspaceContainer)
	{
		return FALSE;
	}

	GetDocumentSessionFilePath(&pszFilePath);
	if (!pszFilePath)
	{
		return FALSE;
	}

	PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
	BOOL bLoaded = DocumentSessionModel_LoadFromFileEx(pszFilePath, &model, &loadStatus);
	if (!bLoaded && loadStatus == PERSIST_LOAD_INVALID_FORMAT)
	{
		PersistFile_QuarantineInvalid(pszFilePath);
	}
	free(pszFilePath);
	if (!bLoaded)
	{
		return FALSE;
	}

	BOOL bRestoredAny = FALSE;
	for (int i = 0; i < model.nEntryCount; ++i)
	{
		if (i == model.nActiveEntry)
		{
			continue;
		}

		BOOL bOpened =
			model.entries[i].nKind == DOCSESSION_ENTRY_FILE ?
			Document_OpenFileInWorkspace(model.entries[i].szFilePath, pWorkspaceContainer) :
			DocumentRecovery_OpenInWorkspace(model.entries[i].szFilePath, pWorkspaceContainer);
		if (bOpened)
		{
			bRestoredAny = TRUE;
		}
	}

	if (model.nActiveEntry >= 0 && model.nActiveEntry < model.nEntryCount)
	{
		BOOL bOpened =
			model.entries[model.nActiveEntry].nKind == DOCSESSION_ENTRY_FILE ?
			Document_OpenFileInWorkspace(model.entries[model.nActiveEntry].szFilePath, pWorkspaceContainer) :
			DocumentRecovery_OpenInWorkspace(model.entries[model.nActiveEntry].szFilePath, pWorkspaceContainer);
		if (bOpened)
		{
			bRestoredAny = TRUE;
		}
	}

	return bRestoredAny;
}

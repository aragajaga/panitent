#include "precomp.h"

#include "documentsessionpersist.h"

#include "document.h"
#include "documentrecovery.h"
#include "documentsessionmodel.h"
#include "documentsessionworkspace.h"
#include "panitentapp.h"
#include "persistfile.h"
#include "recoverystorepersist.h"
#include "viewport.h"
#include "workspacecontainer.h"
#include "shell/pathutil.h"

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

	if (!DocumentSessionWorkspace_CaptureEntries(
		pWorkspaceContainer,
		model.entries,
		ARRAYSIZE(model.entries),
		&model.nEntryCount,
		&model.nActiveEntry,
		(FnDocumentSessionWorkspaceBuildRecoveryPath)DocumentSessionPersist_BuildRecoveryPath,
		NULL))
	{
		return FALSE;
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

	return DocumentSessionWorkspace_RestoreEntries(
		pWorkspaceContainer,
		model.entries,
		model.nEntryCount,
		model.nActiveEntry);
}

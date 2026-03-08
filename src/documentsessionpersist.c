#include "precomp.h"

#include "documentsessionpersist.h"

#include "document.h"
#include "documentsessionmodel.h"
#include "panitentapp.h"
#include "viewport.h"
#include "workspacecontainer.h"
#include "shell/pathutil.h"

static BOOL DocumentSessionPersist_IsFileBackedViewport(ViewportWindow* pViewportWindow)
{
	Document* pDocument = pViewportWindow ? ViewportWindow_GetDocument(pViewportWindow) : NULL;
	return pDocument && Document_IsFilePathSet(pDocument) && Document_GetFilePath(pDocument) && Document_GetFilePath(pDocument)[0] != L'\0';
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

	ViewportWindow* pActiveViewport = WorkspaceContainer_GetCurrentViewport(pWorkspaceContainer);
	for (int i = 0; i < WorkspaceContainer_GetViewportCount(pWorkspaceContainer) && model.nEntryCount < ARRAYSIZE(model.entries); ++i)
	{
		ViewportWindow* pViewportWindow = WorkspaceContainer_GetViewportAt(pWorkspaceContainer, i);
		if (!DocumentSessionPersist_IsFileBackedViewport(pViewportWindow))
		{
			continue;
		}

		Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
		wcscpy_s(
			model.entries[model.nEntryCount].szFilePath,
			ARRAYSIZE(model.entries[model.nEntryCount].szFilePath),
			Document_GetFilePath(pDocument));
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

	BOOL bLoaded = DocumentSessionModel_LoadFromFile(pszFilePath, &model);
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

		if (Document_OpenFileInWorkspace(model.entries[i].szFilePath, pWorkspaceContainer))
		{
			bRestoredAny = TRUE;
		}
	}

	if (model.nActiveEntry >= 0 && model.nActiveEntry < model.nEntryCount)
	{
		if (Document_OpenFileInWorkspace(model.entries[model.nActiveEntry].szFilePath, pWorkspaceContainer))
		{
			bRestoredAny = TRUE;
		}
	}

	return bRestoredAny;
}

#include "precomp.h"

#include "docklayoutpersist.h"

#include "dockhost.h"
#include "dockhostrestore.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockmodelio.h"
#include "dockmodelvalidate.h"
#include "persistfile.h"
#include "panitentapp.h"
#include "shell/pathutil.h"

BOOL PanitentDockLayout_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	PTSTR pszDockLayoutFilePath = NULL;
	GetDockLayoutFilePath(&pszDockLayoutFilePath);
	if (!pszDockLayoutFilePath)
	{
		return FALSE;
	}

	BOOL bResult = PanitentDockLayout_SaveToFilePath(pPanitentApp, pDockHostWindow, pszDockLayoutFilePath);
	free(pszDockLayoutFilePath);
	return bResult;
}

BOOL PanitentDockLayout_SaveToFilePath(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, PCWSTR pszFilePath)
{
	DockModelNode* pModelRoot;
	BOOL bResult;

	UNREFERENCED_PARAMETER(pPanitentApp);

	if (!pDockHostWindow || !pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	pModelRoot = DockModel_CaptureHostLayout(pDockHostWindow);
	if (!pModelRoot)
	{
		return FALSE;
	}

	bResult = DockModelIO_SaveToFile(pModelRoot, pszFilePath);
	DockModel_Destroy(pModelRoot);
	return bResult;
}

BOOL PanitentDockLayout_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const RECT* pDockHostRect)
{
	PTSTR pszDockLayoutFilePath = NULL;
	GetDockLayoutFilePath(&pszDockLayoutFilePath);
	if (!pszDockLayoutFilePath)
	{
		return FALSE;
	}

	BOOL bResult = PanitentDockLayout_RestoreFromFilePath(pPanitentApp, pDockHostWindow, pDockHostRect, pszDockLayoutFilePath);
	free(pszDockLayoutFilePath);
	return bResult;
}

BOOL PanitentDockLayout_RestoreFromFilePath(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const RECT* pDockHostRect, PCWSTR pszFilePath)
{
	DockModelNode* pModelRoot;
	TreeNode* pRootNode;

	if (!pPanitentApp || !pDockHostWindow || !pDockHostRect || !pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
	pModelRoot = DockModelIO_LoadFromFileEx(pszFilePath, &loadStatus);
	if (!pModelRoot)
	{
		if (loadStatus == PERSIST_LOAD_INVALID_FORMAT)
		{
			PersistFile_QuarantineInvalid(pszFilePath);
		}
		return FALSE;
	}
	if (!DockModelValidateAndRepairMainLayout(&pModelRoot, NULL))
	{
		PersistFile_QuarantineInvalid(pszFilePath);
		DockModel_Destroy(pModelRoot);
		return FALSE;
	}

	pRootNode = DockModelBuildTree(pModelRoot);
	DockModel_Destroy(pModelRoot);
	if (!pRootNode || !pRootNode->data)
	{
		DockModelBuildDestroyTree(pRootNode);
		return FALSE;
	}

	((DockData*)pRootNode->data)->rc = *pDockHostRect;

	pPanitentApp->m_pWorkspaceContainer = NULL;
	pPanitentApp->m_pPaletteWindow = NULL;
	PanitentApp_SetOptionBar(pPanitentApp, NULL);

	BOOL bHasWorkspace = FALSE;
	if (!PanitentDockHostRestoreAttachKnownViews(pPanitentApp, pDockHostWindow, pRootNode, &bHasWorkspace))
	{
		DockModelBuildDestroyTree(pRootNode);
		return FALSE;
	}

	DockHostWindow_SetRoot(pDockHostWindow, pRootNode);
	return bHasWorkspace && pPanitentApp->m_pWorkspaceContainer != NULL;
}

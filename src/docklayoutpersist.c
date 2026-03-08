#include "precomp.h"

#include "docklayoutpersist.h"

#include "dockhost.h"
#include "dockhostrestore.h"
#include "dockmodel.h"
#include "dockmodelbuild.h"
#include "dockmodelio.h"
#include "dockmodelvalidate.h"
#include "panitentapp.h"
#include "shell/pathutil.h"

BOOL PanitentDockLayout_Save(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow)
{
	DockModelNode* pModelRoot;
	PTSTR pszDockLayoutFilePath = NULL;
	BOOL bResult;

	UNREFERENCED_PARAMETER(pPanitentApp);

	if (!pDockHostWindow)
	{
		return FALSE;
	}

	pModelRoot = DockModel_CaptureHostLayout(pDockHostWindow);
	if (!pModelRoot)
	{
		return FALSE;
	}

	GetDockLayoutFilePath(&pszDockLayoutFilePath);
	if (!pszDockLayoutFilePath)
	{
		DockModel_Destroy(pModelRoot);
		return FALSE;
	}

	bResult = DockModelIO_SaveToFile(pModelRoot, pszDockLayoutFilePath);
	free(pszDockLayoutFilePath);
	DockModel_Destroy(pModelRoot);
	return bResult;
}

BOOL PanitentDockLayout_Restore(PanitentApp* pPanitentApp, DockHostWindow* pDockHostWindow, const RECT* pDockHostRect)
{
	PTSTR pszDockLayoutFilePath = NULL;
	DockModelNode* pModelRoot;
	TreeNode* pRootNode;

	if (!pPanitentApp || !pDockHostWindow || !pDockHostRect)
	{
		return FALSE;
	}

	GetDockLayoutFilePath(&pszDockLayoutFilePath);
	if (!pszDockLayoutFilePath)
	{
		return FALSE;
	}

	PersistLoadStatus loadStatus = PERSIST_LOAD_IO_ERROR;
	pModelRoot = DockModelIO_LoadFromFileEx(pszDockLayoutFilePath, &loadStatus);
	if (!pModelRoot)
	{
		if (loadStatus == PERSIST_LOAD_INVALID_FORMAT)
		{
			DeleteFileW(pszDockLayoutFilePath);
		}
		free(pszDockLayoutFilePath);
		return FALSE;
	}
	if (!DockModelValidateAndRepairMainLayout(&pModelRoot, NULL))
	{
		DeleteFileW(pszDockLayoutFilePath);
		free(pszDockLayoutFilePath);
		DockModel_Destroy(pModelRoot);
		return FALSE;
	}
	free(pszDockLayoutFilePath);

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

#include "precomp.h"

#include "dockviewfactory.h"

#include "dockshell.h"
#include "glwindow.h"
#include "layerswindow.h"
#include "option_bar.h"
#include "palette_window.h"
#include "panitentapp.h"
#include "toolbox.h"
#include "workspacecontainer.h"

TreeNode* PanitentDockViewFactory_CreateNode(DockNodeRole nRole, PCWSTR pszName)
{
	switch (PanitentDockViewCatalog_Find(nRole, pszName))
	{
	case PNT_DOCK_VIEW_WORKSPACE:
		return DockShell_CreateWorkspaceNode();
	case PNT_DOCK_VIEW_TOOLBOX:
	case PNT_DOCK_VIEW_GLWINDOW:
	case PNT_DOCK_VIEW_PALETTE:
	case PNT_DOCK_VIEW_LAYERS:
	case PNT_DOCK_VIEW_OPTIONBAR:
		return DockShell_CreatePanelNode(pszName);
	default:
		return NULL;
	}
}

Window* PanitentDockViewFactory_CreateWindow(PanitentApp* pPanitentApp, PanitentDockViewId nViewId)
{
	if (!pPanitentApp)
	{
		return NULL;
	}

	switch (nViewId)
	{
	case PNT_DOCK_VIEW_WORKSPACE:
	{
		WorkspaceContainer* pWorkspaceContainer = WorkspaceContainer_Create((Application*)pPanitentApp);
		if (!pPanitentApp->m_pWorkspaceContainer)
		{
			pPanitentApp->m_pWorkspaceContainer = pWorkspaceContainer;
		}
		return (Window*)pWorkspaceContainer;
	}
	case PNT_DOCK_VIEW_TOOLBOX:
		return (Window*)ToolboxWindow_Create((Application*)pPanitentApp);
	case PNT_DOCK_VIEW_GLWINDOW:
		return (Window*)GLWindow_Create((Application*)pPanitentApp);
	case PNT_DOCK_VIEW_PALETTE:
	{
		PaletteWindow* pPaletteWindow = PaletteWindow_Create(pPanitentApp->palette);
		pPanitentApp->m_pPaletteWindow = pPaletteWindow;
		return (Window*)pPaletteWindow;
	}
	case PNT_DOCK_VIEW_LAYERS:
		return (Window*)LayersWindow_Create((Application*)pPanitentApp);
	case PNT_DOCK_VIEW_OPTIONBAR:
	{
		OptionBarWindow* pOptionBarWindow = OptionBarWindow_Create();
		PanitentApp_SetOptionBar(pPanitentApp, pOptionBarWindow);
		OptionBarWindow_SyncTool(pOptionBarWindow, PanitentApp_GetTool(pPanitentApp));
		return (Window*)pOptionBarWindow;
	}
	default:
		return NULL;
	}
}

TreeNode* PanitentDockViewFactory_CreateNodeAndWindow(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	DockNodeRole nRole,
	PCWSTR pszName)
{
	PanitentDockViewId nViewId;
	TreeNode* pNode;
	Window* pWindow;
	DockData* pDockData;

	if (!pPanitentApp || !pDockHostWindow)
	{
		return NULL;
	}

	nViewId = PanitentDockViewCatalog_Find(nRole, pszName);
	if (nViewId == PNT_DOCK_VIEW_NONE)
	{
		return NULL;
	}

	pNode = PanitentDockViewFactory_CreateNode(nRole, pszName);
	if (!pNode || !pNode->data)
	{
		return NULL;
	}

	pWindow = PanitentDockViewFactory_CreateWindow(pPanitentApp, nViewId);
	if (!pWindow || !Window_CreateWindow(pWindow, NULL))
	{
		free(pNode->data);
		free(pNode);
		return NULL;
	}

	pDockData = (DockData*)pNode->data;
	DockData_PinWindow(pDockHostWindow, pDockData, pWindow);
	if (nRole == DOCK_ROLE_WORKSPACE)
	{
		pDockData->bShowCaption = FALSE;
	}

	return pNode;
}

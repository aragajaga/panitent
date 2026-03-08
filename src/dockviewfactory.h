#pragma once

#include "dockhost.h"
#include "dockviewcatalog.h"

typedef struct PanitentApp PanitentApp;

TreeNode* PanitentDockViewFactory_CreateNode(DockNodeRole nRole, PCWSTR pszName);
Window* PanitentDockViewFactory_CreateWindow(PanitentApp* pPanitentApp, PanitentDockViewId nViewId);
TreeNode* PanitentDockViewFactory_CreateNodeAndWindow(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	DockNodeRole nRole,
	PCWSTR pszName);

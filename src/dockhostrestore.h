#pragma once

#include "precomp.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;
typedef struct TreeNode TreeNode;

BOOL PanitentDockHostRestoreAttachKnownViews(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pRootNode,
	BOOL* pbHasWorkspace);

#pragma once

#include "precomp.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;
typedef struct TreeNode TreeNode;
typedef struct DockData DockData;
typedef struct Window Window;

typedef BOOL (*FnDockHostRestoreNodeAttached)(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pNode,
	DockData* pDockData,
	Window* pWindow,
	void* pUserData);

BOOL PanitentDockHostRestoreAttachKnownViews(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pRootNode,
	BOOL* pbHasWorkspace);
BOOL PanitentDockHostRestoreAttachKnownViewsEx(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pRootNode,
	FnDockHostRestoreNodeAttached pfnNodeAttached,
	void* pUserData,
	BOOL* pbHasWorkspace);

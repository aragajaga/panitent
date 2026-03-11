#pragma once

#include "precomp.h"

typedef struct PanitentApp PanitentApp;
typedef struct DockHostWindow DockHostWindow;
typedef struct TreeNode TreeNode;
typedef struct DockData DockData;
typedef struct Window Window;
typedef enum PanitentDockViewId PanitentDockViewId;

typedef Window* (*FnDockHostRestoreResolveView)(
	PanitentApp* pPanitentApp,
	DockHostWindow* pDockHostWindow,
	TreeNode* pNode,
	DockData* pDockData,
	PanitentDockViewId nViewId,
	void* pUserData);

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
	FnDockHostRestoreResolveView pfnResolveView,
	void* pResolveViewUserData,
	FnDockHostRestoreNodeAttached pfnNodeAttached,
	void* pUserData,
	BOOL* pbHasWorkspace);

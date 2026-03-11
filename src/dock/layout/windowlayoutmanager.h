#pragma once

#include "precomp.h"
#include "windowlayoutcatalog.h"

typedef struct PanitentWindow PanitentWindow;
typedef struct DockModelNode DockModelNode;
typedef struct DockFloatingLayoutFileModel DockFloatingLayoutFileModel;
typedef struct FloatingDocumentLayoutModel FloatingDocumentLayoutModel;
typedef int (*FnWindowLayoutManagerMessageSink)(HWND hWndParent, PCWSTR pszText, PCWSTR pszCaption, UINT uType);
typedef BOOL (*FnWindowLayoutManagerSaveProfileSink)(PanitentWindow* pPanitentWindow, uint32_t uId);
typedef BOOL (*FnWindowLayoutManagerSaveCatalogSink)(const void* pCatalog);
typedef BOOL (*FnWindowLayoutManagerPromptSink)(
    HWND hWndParent,
    PCWSTR pszTitle,
    PCWSTR pszPrompt,
    PCWSTR pszInitialName,
    WCHAR* pszName,
    size_t cchName);

void WindowLayoutManager_RefreshApplyMenu(PanitentWindow* pPanitentWindow);
void WindowLayoutManager_SetMessageSink(FnWindowLayoutManagerMessageSink pfnMessageSink);
void WindowLayoutManager_SetSaveProfileSink(FnWindowLayoutManagerSaveProfileSink pfnSaveProfileSink);
void WindowLayoutManager_SetSaveCatalogSink(FnWindowLayoutManagerSaveCatalogSink pfnSaveCatalogSink);
void WindowLayoutManager_SetPromptSink(FnWindowLayoutManagerPromptSink pfnPromptSink);
BOOL WindowLayoutManager_HandleCommand(PanitentWindow* pPanitentWindow, UINT cmdId);
BOOL WindowLayoutManager_MoveCatalogEntry(PanitentWindow* pPanitentWindow, WindowLayoutCatalog* pCatalog, int index, int indexTarget);
BOOL WindowLayoutManager_RenameCatalogEntry(PanitentWindow* pPanitentWindow, WindowLayoutCatalog* pCatalog, int index, PCWSTR pszName);
BOOL WindowLayoutManager_DeleteCatalogEntry(PanitentWindow* pPanitentWindow, WindowLayoutCatalog* pCatalog, int index);
BOOL WindowLayoutManager_ApplyLayoutBundle(
    PanitentWindow* pPanitentWindow,
    DockModelNode* pModelRoot,
    const DockFloatingLayoutFileModel* pFloatingModel,
    const FloatingDocumentLayoutModel* pFloatDocModel);
BOOL WindowLayoutManager_ApplyDefaultLayout(PanitentWindow* pPanitentWindow);

#pragma once

#include "precomp.h"

typedef struct PanitentWindow PanitentWindow;
typedef struct DockModelNode DockModelNode;
typedef struct DockFloatingLayoutFileModel DockFloatingLayoutFileModel;
typedef struct FloatingDocumentLayoutModel FloatingDocumentLayoutModel;
typedef int (*FnWindowLayoutManagerMessageSink)(HWND hWndParent, PCWSTR pszText, PCWSTR pszCaption, UINT uType);
typedef BOOL (*FnWindowLayoutManagerPromptSink)(
    HWND hWndParent,
    PCWSTR pszTitle,
    PCWSTR pszPrompt,
    PCWSTR pszInitialName,
    WCHAR* pszName,
    size_t cchName);

void WindowLayoutManager_RefreshApplyMenu(PanitentWindow* pPanitentWindow);
void WindowLayoutManager_SetMessageSink(FnWindowLayoutManagerMessageSink pfnMessageSink);
void WindowLayoutManager_SetPromptSink(FnWindowLayoutManagerPromptSink pfnPromptSink);
BOOL WindowLayoutManager_HandleCommand(PanitentWindow* pPanitentWindow, UINT cmdId);
BOOL WindowLayoutManager_ApplyLayoutBundle(
    PanitentWindow* pPanitentWindow,
    DockModelNode* pModelRoot,
    const DockFloatingLayoutFileModel* pFloatingModel,
    const FloatingDocumentLayoutModel* pFloatDocModel);
BOOL WindowLayoutManager_ApplyDefaultLayout(PanitentWindow* pPanitentWindow);

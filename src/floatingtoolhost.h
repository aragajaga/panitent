#pragma once

#include "dockhost.h"
#include "dockfloatingmodel.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;
typedef struct PanitentApp PanitentApp;

typedef BOOL (*FnFloatingToolHostWindowCallback)(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    void* pUserData);

BOOL FloatingToolHost_IsPinnedFloatingWindow(HWND hWnd, FloatingWindowContainer** ppFloatingWindowContainer);
BOOL FloatingToolHost_ForEachPinnedWindow(FnFloatingToolHostWindowCallback pfnCallback, void* pUserData);
void FloatingToolHost_DestroyExistingPinnedWindows(void);
BOOL FloatingToolHost_CapturePinnedWindowState(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    DockFloatingLayoutEntry* pEntry);
BOOL FloatingToolHost_RestoreEntry(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    const DockFloatingLayoutEntry* pEntry,
    HWND* phWndFloatingOut);

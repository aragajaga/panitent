#pragma once

#include "floatingdocumenthost.h"

void FloatingDocumentHost_SetCreatePinnedWindowTestHook(FnFloatingDocumentHostCreatePinnedWindowHook pfnHook);
BOOL FloatingDocumentHost_CreatePinnedWindow(
    DockHostWindow* pDockHostTarget,
    HWND hWndChild,
    const RECT* pWindowRect,
    BOOL bStartMove,
    POINT ptMoveScreen,
    HWND* phWndFloatingOut);
BOOL FloatingDocumentHost_CreatePinnedDockHost(
    DockHostWindow* pDockHostTarget,
    PanitentApp* pPanitentApp,
    const RECT* pWindowRect,
    DockHostWindow** ppFloatingDockHostOut,
    HWND* phWndDockHostOut,
    HWND* phWndFloatingOut);

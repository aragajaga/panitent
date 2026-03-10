#pragma once

#include "floatingtoolhost.h"

BOOL FloatingToolHost_RestoreEntry(
    PanitentApp* pPanitentApp,
    DockHostWindow* pDockHostWindow,
    const DockFloatingLayoutEntry* pEntry,
    HWND* phWndFloatingOut);

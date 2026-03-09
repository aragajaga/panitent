#pragma once

#include "precomp.h"

typedef struct PanitentWindow PanitentWindow;

void WindowLayoutManager_RefreshApplyMenu(PanitentWindow* pPanitentWindow);
BOOL WindowLayoutManager_HandleCommand(PanitentWindow* pPanitentWindow, UINT cmdId);

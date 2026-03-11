#pragma once

#include "dockhost.h"
#include "dockfloatingmodel.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;
typedef struct PanitentApp PanitentApp;

typedef BOOL (*FnFloatingToolHostWindowCallback)(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    void* pUserData);

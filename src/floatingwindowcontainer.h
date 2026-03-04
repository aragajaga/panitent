#pragma once

#include "precomp.h"

#include "win32/window.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;
typedef struct DockHostWindow DockHostWindow;

struct FloatingWindowContainer {
	Window base;
	BOOL bPinned;
	BOOL fCaptionUnpinStarted;
	POINT ptCaptionUnpinStartingPoint;
	HWND hWndPrevParent;
	HWND hWndChild;
	DockHostWindow* pDockHostTarget;
	int iDockSizeHint;
};

FloatingWindowContainer* FloatingWindowContainer_Create();
void FloatingWindowContainer_PinWindow(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndChild);
void FloatingWindowContainer_SetDockTarget(FloatingWindowContainer* pFloatingWindowContainer, DockHostWindow* pDockHostWindow);

#pragma once

#include "precomp.h"

#include "win32/window.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;

struct FloatingWindowContainer {
	Window base;
	BOOL bPinned;
	BOOL fCaptionUnpinStarted;
	POINT ptCaptionUnpinStartingPoint;
	HWND hWndPrevParent;
	HWND hWndChild;
};

FloatingWindowContainer* FloatingWindowContainer_Create();
void FloatingWindowContainer_PinWindow(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndChild);

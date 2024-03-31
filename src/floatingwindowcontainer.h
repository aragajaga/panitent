#pragma once

#include "precomp.h"

#include "win32/window.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;

struct FloatingWindowContainer {
	Window base;
	BOOL bPinned;
	BOOL fCaptionUnpinStarted;
	POINT ptCaptionUnpinStartingPoint;
	HWND hWndChild;
};

FloatingWindowContainer* FloatingWindowContainer_Create(Application*);
void FloatingWindowContainer_PinWindow(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndChild);

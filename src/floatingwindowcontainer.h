#pragma once

#include "precomp.h"

#include "win32/window.h"

typedef struct FloatingWindowContainer FloatingWindowContainer;
typedef struct DockHostWindow DockHostWindow;

#define FLOAT_DOCK_POLICY_PANEL 0
#define FLOAT_DOCK_POLICY_DOCUMENT 1

struct FloatingWindowContainer {
	Window base;
	BOOL bPinned;
	BOOL fCaptionUnpinStarted;
	POINT ptCaptionUnpinStartingPoint;
	BOOL bNcTracking;
	int nCaptionButtonHot;
	int nCaptionButtonPressed;
	HWND hWndPrevParent;
	HWND hWndChild;
	DockHostWindow* pDockHostTarget;
	int iDockSizeHint;
	int nDockPolicy;
};

FloatingWindowContainer* FloatingWindowContainer_Create();
void FloatingWindowContainer_PinWindow(FloatingWindowContainer* pFloatingWindowContainer, HWND hWndChild);
void FloatingWindowContainer_SetDockTarget(FloatingWindowContainer* pFloatingWindowContainer, DockHostWindow* pDockHostWindow);
void FloatingWindowContainer_SetDockPolicy(FloatingWindowContainer* pFloatingWindowContainer, int nDockPolicy);

#pragma once

#include "win32/window.h"

typedef struct _Document Document;

typedef struct ViewportWindow ViewportWindow;
struct ViewportWindow {
	Window base;

	Document* document;
	POINT ptOffset;
	POINT ptDrag;
	BOOL bDrag;
	float fZoom;
	HBRUSH hbrChecker;
};

ViewportWindow* ViewportWindow_Create();
Document* ViewportWindow_GetDocument(ViewportWindow* pViewportWindow);
void ViewportWindow_SetDocument(ViewportWindow* pViewportWindow, Document* document);
void ViewportWindow_ClientToCanvas(ViewportWindow* pViewportWindow, int x, int y, LPPOINT lpPt);

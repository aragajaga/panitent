#pragma once

#include "win32/window.h"
#include <stddef.h>
#include <stdint.h>

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
	HFONT hFontTextOverlay;
	POINT ptTextOverlayCanvas;
	RECT rcTextOverlayClient;
	int nTextOverlayFontDocPx;
	int nTextOverlayFontClientPx;
	int nTextOverlayLineAdvanceClient;
	int nTextOverlayPreferredColumn;
	size_t cchTextOverlay;
	size_t cchTextOverlayCapacity;
	size_t nTextOverlayCaret;
	WCHAR* pszTextOverlay;
	uint32_t textOverlayColor;
	BOOL bTextOverlayCaretVisible;
	UINT_PTR uTextOverlayCaretTimerId;
};

ViewportWindow* ViewportWindow_Create();
Document* ViewportWindow_GetDocument(ViewportWindow* pViewportWindow);
void ViewportWindow_SetDocument(ViewportWindow* pViewportWindow, Document* document);
void ViewportWindow_ClientToCanvas(ViewportWindow* pViewportWindow, int x, int y, LPPOINT lpPt);
void ViewportWindow_CanvasToClient(ViewportWindow* pViewportWindow, int x, int y, LPPOINT lpPt);
BOOL ViewportWindow_HasTextOverlay(ViewportWindow* pViewportWindow);
BOOL ViewportWindow_BeginTextOverlay(ViewportWindow* pViewportWindow, int xCanvas, int yCanvas, uint32_t color);
void ViewportWindow_CommitTextOverlay(ViewportWindow* pViewportWindow);
void ViewportWindow_CancelTextOverlay(ViewportWindow* pViewportWindow);

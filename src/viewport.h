#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "precomp.h"

#include "panitent.h"

#include "document.h"
#include "tool.h"
#include "canvas.h"

#define VIEWPORTCTL_ID 100
#define WC_VIEWPORT L"Win32Class_Viewport"

typedef struct _Viewport {
  Document* document;
  HWND hwnd;
  POINT offset;
  POINT drag;
  POINT scaleCenter;
  BOOL fDrag;
  float scale;
} Viewport;

void Viewport_Invalidate(Viewport* viewport);

BOOL Viewport_RegisterClass(HINSTANCE hInstance);
void Viewport_SetDocument(Viewport* viewport, Document* document);
Document* Viewport_GetDocument(Viewport* viewport);

#endif /* VIEWPORT_H */

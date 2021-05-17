#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "panitent.h"

#include "document.h"
#include "canvas.h"

typedef struct _Viewport Viewport;

extern const WCHAR szViewportWndClass[];

Viewport* Viewport_Create(HWND, Document*);
void Viewport_Invalidate(Viewport* viewport);
BOOL Viewport_Register(HINSTANCE);
HWND Viewport_GetHWND(Viewport*);

void Viewport_SetDocument(Viewport*, Document*);
Document* Viewport_GetDocument(Viewport*);

#endif /* VIEWPORT_H */

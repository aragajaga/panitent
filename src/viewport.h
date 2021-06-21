#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "precomp.h"

#include "panitent.h"

#include "document.h"
#include "tool.h"
#include "canvas.h"

#define VIEWPORTCTL_ID 100
#define VIEWPORTCTL_WC L"CanvasCtlClass"

typedef struct _Viewport {
  Document* document;
  ATOM wndclass;
  HWND hwnd;
  int canvas_x;
  int canvas_y;
  BOOL view_dragging;
} Viewport;

extern Viewport g_viewport;

void Viewport_RegisterClass();
void Viewport_Invalidate();

typedef struct _tagTRECT {
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
} TRECT;

typedef struct _tagIMAGE {
  void* data;
  TRECT rc;
} IMAGE;

typedef struct _tagCANVAS {
  IMAGE img;
} CANVAS;

LRESULT CALLBACK Viewport_WndProc(HWND hwnd,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam);
void CanvasFillSolid(IMAGE* img, COLORREF color);
void CanvasFillTest(IMAGE* img);
void CanvasWuLinesTest();
void CreateCanvas(IMAGE* img, UINT uWidth, UINT uHeight);
void GetCanvasRect(IMAGE* img, RECT* rcCanvas);
void PNTRectangle(IMAGE* img, int x1, int y1, int x2, int y2);
void Viewport_RegisterClass();
void Viewport_OnLButtonDown(MOUSEEVENT mEvt);
void Viewport_OnLButtonUp(MOUSEEVENT mEvt);
void Viewport_OnMouseMove(MOUSEEVENT mEvt);
void Viewport_OnPaint(HWND hWnd);
void image_init(IMAGE* img);
void Viewport_Init(Viewport* vp);
void Viewport_SetDocument(Document*);

#endif /* VIEWPORT_H */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "precomp.h"

#include "panitent.h"

#include "document.h"
#include "tool.h"
#include "canvas.h"

#define VIEWPORTCTL_ID 100
#define VIEWPORTCTL_WC L"CanvasCtlClass"

typedef struct _viewport {
  document_t* document;
  ATOM wndclass;
  HWND hwnd;
  int canvas_x;
  int canvas_y;
  BOOL view_dragging;
} viewport_t;

extern viewport_t g_viewport;

void viewport_register_class();
void viewport_invalidate();

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

LRESULT CALLBACK viewport_wndproc(HWND hwnd,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam);
void CanvasFillSolid(IMAGE* img, COLORREF color);
void CanvasFillTest(IMAGE* img);
void CanvasWuLinesTest();
void CreateCanvas(IMAGE* img, UINT uWidth, UINT uHeight);
void GetCanvasRect(IMAGE* img, RECT* rcCanvas);
void PNTRectangle(IMAGE* img, int x1, int y1, int x2, int y2);
void viewport_register_class();
void viewport_onlbuttondown(MOUSEEVENT mEvt);
void viewport_onlbuttonup(MOUSEEVENT mEvt);
void viewport_onmousemove(MOUSEEVENT mEvt);
void viewport_onpaint(HWND hWnd);
void image_init(IMAGE* img);
void viewport_init(viewport_t* vp);
void viewport_set_document(document_t*);

#endif /* VIEWPORT_H */

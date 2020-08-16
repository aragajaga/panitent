#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "stdafx.h"

#include "document.h"
#include "tool.h"
#include "canvas.h"

#define VIEWPORTCTL_ID 100
#define VIEWPORTCTL_WC L"CanvasCtlClass"

typedef struct _viewport {
  document_t* document;
  ATOM win_class;
  HWND win_handle;
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
    void *data;
    TRECT rc;
} IMAGE;

typedef struct _tagCANVAS {
    IMAGE img;
} CANVAS;

typedef struct _tagVIEWPORT {
    IMAGE   img;
    size_t  seqi;
    IMAGE   seq[8];
    TOOL    tool;
    
    HWND    hwnd;
} VIEWPORT;

LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CanvasFillSolid(IMAGE *img, COLORREF color);
void CanvasFillTest(IMAGE *img);
void CanvasWuLinesTest();
void CreateCanvas(IMAGE *img, UINT uWidth, UINT uHeight);
void GetCanvasRect(IMAGE *img, RECT *rcCanvas);
void ImageAlloc(IMAGE *img);
void ImageFree(IMAGE *img);
void PNTRectangle(IMAGE *img, int x1, int y1, int x2, int y2);
void RegisterViewportCtl();
void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt);
void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt);
void ViewportCtl_OnMouseMove(MOUSEEVENT mEvt);
void ViewportCtl_OnPaint(HWND hWnd);
void WuCircle(IMAGE *img, int offset_x, int offset_y, int r);
void WuLine(IMAGE *img, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void image_init(IMAGE *img);
void viewport_init(VIEWPORT* vp);

#endif /* VIEWPORT_H */

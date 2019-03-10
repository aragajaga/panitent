#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "stdafx.h"
#include "tool.h"

#define VIEWPORTCTL_ID 100
#define VIEWPORTCTL_WC L"CanvasCtlClass"

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
    CANVAS  cvs; // Исходное изображение
    IMAGE   img; // Увеличенное/кропленое
    TOOL    tool;
    
    HWND    hwnd;
} VIEWPORT;

void CreateCanvas(UINT uWidth, UINT uHeight);
void image_init(IMAGE *img);
void viewport_init(VIEWPORT* vp);
void RegisterViewportCtl();
void CanvasFillTest(IMAGE *img);
void CanvasFillSolid(IMAGE *img, COLORREF color);
void CanvasWuLinesTest();
void GetCanvasRect(RECT *rcCanvas);
void ViewportCtl_OnPaint(HWND hWnd);
void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt);
void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt);
void ViewportCtl_OnMouseMove(MOUSEEVENT mEvt);
void WuLine(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void WuCircle(int offset_x, int offset_y, int r);
void PNTRectangle(int x1, int y1, int x2, int y2);
LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* VIEWPORT_H */

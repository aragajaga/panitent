#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "stdafx.h"

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
    
    HWND    hwnd;
} VIEWPORT;

typedef struct _tagMOUSEEVENT MOUSEEVENT;
struct _tagMOUSEEVENT
{
    HWND hwnd;
    LPARAM lParam;
};

void image_init(IMAGE *img);
void viewport_init(VIEWPORT* vp);
void RegisterViewportCtl();
void ViewportCtl_OnPaint(HWND hWnd);
void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt);
void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt);
void ViewportCtl_OnMouseMove(MOUSEEVENT mEvt);
LRESULT CALLBACK _viewport_msgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* VIEWPORT_H */

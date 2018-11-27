#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "stdafx.h"

#define CANVASCTL_ID 100
#define CANVASCTL_WC L"CanvasCtlClass"

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

void image_init(IMAGE *img);

typedef struct _tagCANVASCTL {
    IMAGE *img;
} CANVASCTL;

typedef struct _tagMOUSEEVENT MOUSEEVENT;
struct _tagMOUSEEVENT
{
    HWND hwnd;
    LPARAM lParam;
};

void CreateImg();
void RegisterCanvasCtl();
void CanvasCtl_OnPaint(HWND hWnd);
void CanvasCtl_OnLButtonDown(MOUSEEVENT mEvt);
void CanvasCtl_OnLButtonUp(MOUSEEVENT mEvt);
void CanvasCtl_OnMouseMove(MOUSEEVENT mEvt);
LRESULT CALLBACK _canvas_msgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* VIEWPORT_H */

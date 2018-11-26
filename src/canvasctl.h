#ifndef CANVASCTL_H
#define CANVASCTL_H

#include "stdafx.h"

#define CANVASCTL_ID 100
#define CANVASCTL_WC L"CanvasCtlClass"

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

#endif /* CANVASCTL_H */

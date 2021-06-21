#ifndef PANITENT_TOOLBOX_H
#define PANITENT_TOOLBOX_H

#include "precomp.h"
#include "panitent.h"
#include "tool.h"

#define TOOLBOX_WC L"ToolBoxClass"

typedef struct _Toolbox {
  Tool* tools;
  unsigned int tool_count;
  HWND hwnd;
} Toolbox;

void Toolbox_OnPaint(Toolbox*);
void Toolbox_OnMouseMove(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK Toolbox_WndProc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam);
void Toolbox_RegisterClass();
void Toolbox_UnregisterClass();

void ToolPointer_OnLButtonUp(MOUSEEVENT mEvt);
void ToolPointer_OnLButtonDown(MOUSEEVENT mEvt);
void ToolPointer_OnMouseMove(MOUSEEVENT mEvt);
void ToolPointer_Init();

void ToolPencil_OnLButtonUp(MOUSEEVENT mEvt);
void ToolPencil_OnLButtonDown(MOUSEEVENT mEvt);
void ToolPencil_OnMouseMove(MOUSEEVENT mEvt);
void ToolPencil_Init();

void ToolCircle_OnLButtonUp(MOUSEEVENT mEvt);
void ToolCircle_OnLButtonDown(MOUSEEVENT mEvt);
void ToolCircle_OnMouseMove(MOUSEEVENT mEvt);
void ToolCircle_Init();

void ToolLine_OnLButtonUp(MOUSEEVENT mEvt);
void ToolLine_OnLButtonDown(MOUSEEVENT mEvt);
void ToolLine_OnMouseMove(MOUSEEVENT mEvt);
void ToolLine_Init();

void ToolRectangle_OnLButtonUp(MOUSEEVENT mEvt);
void ToolRectangle_OnLButtonDown(MOUSEEVENT mEvt);
void ToolRectangle_OnMouseMove(MOUSEEVENT mEvt);
void ToolRectangle_Init();

void ToolText_OnLButtonUp(MOUSEEVENT mEvt);
void ToolText_OnLButtonDown(MOUSEEVENT mEvt);
void ToolText_OnLMouseMove(MOUSEEVENT mEvt);
void ToolText_Init();

void ToolFill_Init();
void ToolPicker_Init();

#endif /* PANITENT_TOOLBOX_H */

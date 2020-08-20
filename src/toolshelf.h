#ifndef PANITENT_TOOLSHELF_H
#define PANITENT_TOOLSHELF_H

#include "precomp.h"
#include "panitent.h"

#define TOOLSHELF_WC L"ToolShelfClass"

void ToolShelf_OnPaint(HWND hwnd);
void ToolShelf_OnMouseMove(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK _toolshelf_msgproc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
void RegisterToolShelf();

void Pencil_OnLButtonUp(MOUSEEVENT mEvt);
void Pencil_OnLButtonDown(MOUSEEVENT mEvt);
void Pencil_OnMouseMove(MOUSEEVENT mEvt);
void Pointer_Init();

void Pencil_OnLButtonUp(MOUSEEVENT mEvt);
void Pencil_OnLButtonDown(MOUSEEVENT mEvt);
void Pencil_OnMouseMove(MOUSEEVENT mEvt);
void Pencil_Init();

void Circle_OnLButtonUp(MOUSEEVENT mEvt);
void Circle_OnLButtonDown(MOUSEEVENT mEvt);
void Circle_OnMouseMove(MOUSEEVENT mEvt);
void Circle_Init();

void Line_OnLButtonUp(MOUSEEVENT mEvt);
void Line_OnLButtonDown(MOUSEEVENT mEvt);
void Line_OnMouseMove(MOUSEEVENT mEvt);
void Line_Init();

void Rectangle_OnLButtonUp(MOUSEEVENT mEvt);
void Rectangle_OnLButtonDown(MOUSEEVENT mEvt);
void Rectangle_OnMouseMove(MOUSEEVENT mEvt);
void Rectangle_Init();

void Text_OnLButtonUp(MOUSEEVENT mEvt);
void Text_OnLButtonDown(MOUSEEVENT mEvt);
void Text_OnMouseMove(MOUSEEVENT mEvt);
void Text_Init();

#endif  /* PANITENT_TOOLSHELF_H */

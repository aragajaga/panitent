#ifndef PANITENT_TOOLBOX_H
#define PANITENT_TOOLBOX_H

#include "precomp.h"
#include "panitent.h"
#include "tool.h"

#define TOOLBOX_WC L"ToolBoxClass"

typedef struct _toolbox_t 
{
  tool_t *tools;
  unsigned int tool_count;
  HWND hwnd;
} toolbox_t;

void toolbox_onpaint(toolbox_t *);
void toolbox_onmousemove(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK toolbox_wndproc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
void toolbox_register_class();
void toolbox_unregister_class();

void tool_pointer_onlbuttonup(MOUSEEVENT mEvt);
void tool_pointer_onlbuttondown(MOUSEEVENT mEvt);
void tool_pointer_onmousemove(MOUSEEVENT mEvt);
void tool_pointer_init();

void tool_pencil_onlbuttonup(MOUSEEVENT mEvt);
void tool_pencil_onlbuttondown(MOUSEEVENT mEvt);
void tool_pencil_onmousemove(MOUSEEVENT mEvt);
void tool_pencil_init();

void tool_circle_onlbuttonup(MOUSEEVENT mEvt);
void tool_circle_onlbuttondown(MOUSEEVENT mEvt);
void tool_circle_onmousemove(MOUSEEVENT mEvt);
void tool_circle_init();

void tool_line_onlbuttonup(MOUSEEVENT mEvt);
void tool_line_onlbuttondown(MOUSEEVENT mEvt);
void tool_line_onmousemove(MOUSEEVENT mEvt);
void tool_line_init();

void tool_rectangle_onlbuttonup(MOUSEEVENT mEvt);
void tool_rectangle_onlbuttondown(MOUSEEVENT mEvt);
void tool_rectangle_onmousemove(MOUSEEVENT mEvt);
void tool_rectangle_init();

void tool_text_onlbuttonup(MOUSEEVENT mEvt);
void tool_text_onlbuttondown(MOUSEEVENT mEvt);
void tool_text_onmousemove(MOUSEEVENT mEvt);
void tool_text_init();

#endif  /* PANITENT_TOOLBOX_H */

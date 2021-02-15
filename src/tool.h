#ifndef PANITENT_TOOL_H
#define PANITENT_TOOL_H

#include "precomp.h"
#include "panitent.h"

typedef struct _tool_t {
  wchar_t* label;
  int img;
  void (*onlbuttonup)(MOUSEEVENT mEvt);
  void (*onlbuttondown)(MOUSEEVENT mEvt);
  void (*onmousemove)(MOUSEEVENT mEvt);
} tool_t;

#endif /* PANITENT_TOOL_H */

#ifndef PANITENT_TOOL_H
#define PANITENT_TOOL_H

#include "precomp.h"
#include "panitent.h"

typedef struct _Tool {
  wchar_t* label;
  int img;
  void (*onlbuttonup)(MOUSEEVENT mEvt);
  void (*onlbuttondown)(MOUSEEVENT mEvt);
  void (*onrbuttonup)(MOUSEEVENT mEvt);
  void (*onmousemove)(MOUSEEVENT mEvt);
} Tool;

#endif /* PANITENT_TOOL_H */

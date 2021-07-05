#ifndef PANITENT_TOOL_H
#define PANITENT_TOOL_H

#include "precomp.h"
#include "panitent.h"

typedef struct _Tool {
  wchar_t* label;
  int img;
  void (*onlbuttonup)(HWND hwnd, WPARAM wParam, LPARAM lParam);
  void (*onlbuttondown)(HWND hwnd, WPARAM wParam, LPARAM lParam);
  void (*onrbuttonup)(HWND hwnd, WPARAM wParam, LPARAM lParam);
  void (*onmousemove)(HWND hwnd, WPARAM wParam, LPARAM lParam);
} Tool;

#endif /* PANITENT_TOOL_H */

#pragma once

typedef struct ViewportWindow ViewportWindow;

typedef struct Tool Tool;
struct Tool {
  wchar_t* pszLabel;
  int img;
  void (*OnLButtonUp)(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
  void (*OnLButtonDown)(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
  void (*OnRButtonUp)(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
  void (*OnMouseMove)(Tool* pTool, ViewportWindow* pViewportWindow, int x, int y, UINT keyFlags);
};

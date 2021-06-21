#include "precomp.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include "file_open.h"
#include "document.h"
#include "viewport.h"
#include "palette.h"
#include "canvas.h"
#include "debug.h"
#include "tool.h"

Viewport g_viewport;
Tool g_tool;

void swapf(float* a, float* b)
{
  float c = *a;
  *b      = *a;
  *a      = c;
}

void Viewport_Invalidate()
{
  InvalidateRect(g_viewport.hwnd, NULL, TRUE);
}

void Viewport_SetDocument(Document* document)
{
  g_viewport.document = document;
  InvalidateRect(g_viewport.hwnd, NULL, TRUE);
}

void Viewport_RegisterClass()
{
  /*Break if already registered */
  if (g_viewport.wndclass)
    return;

  WNDCLASSEX wcex  = {0};
  wcex.cbSize      = sizeof(WNDCLASSEX);
  wcex.style       = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)Viewport_WndProc;
  wcex.hInstance   = GetModuleHandle(NULL);
  wcex.hCursor     = LoadCursor(NULL, IDC_ARROW);
  /* Cause flickering */
  /* wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); */
  wcex.lpszClassName = VIEWPORTCTL_WC;

  ATOM class_atom = RegisterClassEx(&wcex);
  if (!class_atom) {
    MessageBox(NULL, L"Failed to register class!", NULL, MB_OK | MB_ICONERROR);
    return;
  }

  g_viewport.wndclass = class_atom;
}

void Viewport_GetCanvasRect(Viewport* vp, IMAGE* img, RECT* rcCanvas)
{
  RECT rcViewport = {0};
  if (GetClientRect(g_viewport.hwnd, &rcViewport)) {
    printf("[ViewportRect] w: %ld h: %ld\n",
           rcViewport.right,
           rcViewport.bottom);
  } else {
    printf("[GetCanvasRect] Failed to get viewport rect\n");
  }
  GetClientRect(g_viewport.hwnd, &rcViewport);

  rcCanvas->left   = vp->canvas_x;
  rcCanvas->top    = vp->canvas_y;
  rcCanvas->right  = img->rc.width;
  rcCanvas->bottom = img->rc.height;

  printf("[GetCanvasRect] right: %ld\n", rcCanvas->right);
  printf("[GetCanvasRect] bottom: %ld\n", rcCanvas->bottom);
}

void Viewport_OnCreate()
{
  g_viewport.canvas_x = 0;
  g_viewport.canvas_y = 0;
}

BOOL gdi_blit_canvas(HDC hdc, int x, int y, Canvas* canvas)
{
  HBITMAP hbitmap = CreateBitmap(canvas->width,
                                 canvas->height,
                                 1,
                                 sizeof(uint32_t) * 8,
                                 canvas->buffer);

  HDC bitmapdc;

  bitmapdc = CreateCompatibleDC(hdc);
  SelectObject(bitmapdc, hbitmap);

  BOOL status =
      BitBlt(hdc, x, y, canvas->width, canvas->height, bitmapdc, 0, 0, SRCCOPY);

  if (!status)
    MessageBox(NULL, L"BitBlt failed", NULL, MB_OK | MB_ICONERROR);

  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  return TRUE;
}

void Viewport_OnPaint(HWND hwnd)
{
  if (g_viewport.document == NULL)
    return;

  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  gdi_blit_canvas(hdc, 0, 0, g_viewport.document->canvas);
  /*
  RECT rc = {0};
  rc.left = 0;
  rc.top = 0;
  rc.right = g_viewport.document->canvas->width + 1;
  rc.bottom = g_viewport.document->canvas->height + 1;
  */

  MoveToEx(hdc, g_viewport.document->canvas->width, 0, NULL);
  LineTo(hdc, g_viewport.document->canvas->width,
      g_viewport.document->canvas->height);
  LineTo(hdc, 0, g_viewport.document->canvas->height);
  EndPaint(hwnd, &ps);
}

void Viewport_OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (wParam == VK_SPACE) {
    g_viewport.view_dragging = TRUE;
  }
}

void Viewport_OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (wParam == VK_SPACE) {
    g_viewport.view_dragging = FALSE;
  }
}

void Viewport_OnMouseWheel(WPARAM wParam)
{
}

void Viewport_OnLButtonDown(MOUSEEVENT mEvt)
{
  if (g_tool.onlbuttondown != NULL) {
    g_tool.onlbuttondown(mEvt);
  }
}

void Viewport_OnLButtonUp(MOUSEEVENT mEvt)
{
  if (g_tool.onlbuttonup != NULL) {
    g_tool.onlbuttonup(mEvt);
  }
}

void Viewport_OnRButtonUp(MOUSEEVENT mEvt)
{
  if (g_tool.onrbuttonup != NULL) {
    g_tool.onrbuttonup(mEvt);
  }
}

void Viewport_OnMouseMove(MOUSEEVENT mEvt)
{
  if (g_viewport.view_dragging) {
    g_viewport.canvas_x = LOWORD(mEvt.lParam);
    g_viewport.canvas_y = HIWORD(mEvt.lParam);
  }

  if (g_tool.onmousemove) {
    g_tool.onmousemove(mEvt);
  }
}

LRESULT CALLBACK Viewport_WndProc(HWND hwnd,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  MOUSEEVENT mevt;
  mevt.hwnd   = hwnd;
  mevt.lParam = lParam;

  switch (msg) {
  case WM_CREATE:
    Viewport_OnCreate();
    break;
  case WM_PAINT:
    Viewport_OnPaint(hwnd);
    break;
  case WM_KEYDOWN:
    Viewport_OnKeyDown(hwnd, wParam, lParam);
    break;
  case WM_MOUSEWHEEL:
    Viewport_OnMouseWheel(wParam);
    break;
  case WM_LBUTTONDOWN:
    Viewport_OnLButtonDown(mevt);
    break;
  case WM_LBUTTONUP:
    Viewport_OnLButtonUp(mevt);
    break;
  case WM_RBUTTONUP:
    Viewport_OnRButtonUp(mevt);
    break;
  case WM_MOUSEMOVE:
    Viewport_OnMouseMove(mevt);
    break;
  case WM_SIZE:
    Viewport_Invalidate();
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

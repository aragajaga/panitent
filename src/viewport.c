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

viewport_t g_viewport;
tool_t g_tool;

void swapf(float* a, float* b)
{
  float c = *a;
  *b      = *a;
  *a      = c;
}

void viewport_invalidate()
{
  InvalidateRect(g_viewport.hwnd, NULL, FALSE);
}

void viewport_set_document(document_t* document)
{
  g_viewport.document = document;
}

void viewport_register_class()
{
  /*Break if already registered */
  if (g_viewport.wndclass)
    return;

  WNDCLASSEX wcex  = {0};
  wcex.cbSize      = sizeof(WNDCLASSEX);
  wcex.style       = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)viewport_wndproc;
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

void viewport_get_canvas_rect(viewport_t* vp, IMAGE* img, RECT* rcCanvas)
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

void viewport_oncreate()
{
  g_viewport.canvas_x = 0;
  g_viewport.canvas_y = 0;
}

BOOL gdi_blit_canvas(HDC hdc, int x, int y, canvas_t* canvas)
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

void viewport_onpaint(HWND hwnd)
{
  if (g_viewport.document == NULL)
    return;

  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  gdi_blit_canvas(hdc, 0, 0, g_viewport.document->canvas);
  EndPaint(hwnd, &ps);
}

void viewport_onkeydown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (wParam == VK_SPACE) {
    g_viewport.view_dragging = TRUE;
  }
}

void viewport_onkeyup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (wParam == VK_SPACE) {
    g_viewport.view_dragging = FALSE;
  }
}

void viewport_onmousewheel(WPARAM wParam)
{
}

void viewport_onlbuttondown(MOUSEEVENT mEvt)
{
  if (g_tool.onlbuttondown != NULL) {
    g_tool.onlbuttondown(mEvt);
  }
}

void viewport_onlbuttonup(MOUSEEVENT mEvt)
{
  if (g_tool.onlbuttonup != NULL) {
    g_tool.onlbuttonup(mEvt);
  }
}

void viewport_onmousemove(MOUSEEVENT mEvt)
{
  if (g_viewport.view_dragging) {
    g_viewport.canvas_x = LOWORD(mEvt.lParam);
    g_viewport.canvas_y = HIWORD(mEvt.lParam);
  }

  if (g_tool.onmousemove) {
    g_tool.onmousemove(mEvt);
  }
}

LRESULT CALLBACK viewport_wndproc(HWND hwnd,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  MOUSEEVENT mevt;
  mevt.hwnd   = hwnd;
  mevt.lParam = lParam;

  switch (msg) {
  case WM_CREATE:
    viewport_oncreate();
    break;
  case WM_PAINT:
    viewport_onpaint(hwnd);
    break;
  case WM_KEYDOWN:
    viewport_onkeydown(hwnd, wParam, lParam);
    break;
  case WM_MOUSEWHEEL:
    viewport_onmousewheel(wParam);
    break;
  case WM_LBUTTONDOWN:
    viewport_onlbuttondown(mevt);
    break;
  case WM_LBUTTONUP:
    viewport_onlbuttonup(mevt);
    break;
  case WM_MOUSEMOVE:
    viewport_onmousemove(mevt);
    break;
  case WM_SIZE:
    viewport_invalidate();
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

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

void swapf(float *a, float *b)
{
    float c = *a;
    *b = *a;
    *a = c;
}

void viewport_invalidate()
{
  InvalidateRect(g_viewport.hwnd, NULL, TRUE);
}

void ViewportUpdate()
{
    InvalidateRect(g_viewport.hwnd, NULL, TRUE);
    printf("[Viewport] View updated\n");
}

void viewport_set_document(document_t* document)
{
  g_viewport.document = document;
}

void viewport_register_class()
{
  /* Break if already registered */
  if (g_viewport.win_class)
    return;

  WNDCLASSEX wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)ViewportWndProc;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  // wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = VIEWPORTCTL_WC;

  ATOM class_atom = RegisterClassEx(&wcex);
  if (!class_atom) {
    MessageBox(NULL, L"Failed to register class!", NULL,
        MB_OK | MB_ICONERROR);
    return;
  }

  g_viewport.win_class = class_atom;
}

void RegisterViewportCtl()
{
    WNDCLASS wc = {0};

    wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)ViewportWndProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = VIEWPORTCTL_WC;

    if (RegisterClass(&wc))
    {
        printf("[Viewport] Window class registered\n");
    }
    else {
        printf("[Viewport] Failed to register window class\n");
    }
}

int cvsx;
int cvsy;

void GetCanvasRect(IMAGE *img, RECT *rcCanvas)
{
    RECT rcViewport = {0};
    if (GetClientRect(g_viewport.hwnd, &rcViewport))
    {
        printf("[ViewportRect] w: %ld h: %ld\n", rcViewport.right, rcViewport.bottom);
    }
    else {
        printf("[GetCanvasRect] Failed to get viewport rect\n");
    }
    GetClientRect(g_viewport.hwnd, &rcViewport);

    rcCanvas->left      = cvsx;
    rcCanvas->top       = cvsy;
    rcCanvas->right     = img->rc.width;
    rcCanvas->bottom    = img->rc.height;
    
    printf("[GetCanvasRect] right: %ld\n", rcCanvas->right);
    printf("[GetCanvasRect] bottom: %ld\n", rcCanvas->bottom);
}

void ViewportCtl_OnCreate()
{
    cvsx = 0;
    cvsy = 0;
    
    /* [vp.seq] Memory leak cause if window will be re-created */
    g_viewport.seqi = 0;
}

BOOL gdi_blit_canvas(HDC hdc, int x, int y, canvas_t* canvas)
{
  HBITMAP hbitmap = CreateBitmap(canvas->width, canvas->height, 1,
      sizeof(uint32_t) * 8, canvas->buffer);

  HDC bitmapdc;

  bitmapdc = CreateCompatibleDC(hdc);
  SelectObject(bitmapdc, hbitmap);

  BOOL status = BitBlt(hdc, x, y, canvas->width, canvas->height, bitmapdc, 0, 0,
      SRCCOPY);

  if (!status)
    MessageBox(NULL, L"BitBlt failed", NULL, MB_OK | MB_ICONERROR);
  
  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  return TRUE;
}

void ViewportCtl_OnPaint(HWND hwnd)
{
  if (g_viewport.document == NULL)
    return;

  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  gdi_blit_canvas(hdc, 0, 0, g_viewport.document->canvas);
  EndPaint(hwnd, &ps);
}

BOOL bViewDragKey;
BOOL bViewDrag;
POINT dragPrev;

void ViewportCtl_OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_SPACE)
    {
        printf("[ViewportCtl] KeyDown VK_SPACE\n");
        bViewDragKey = TRUE;
    }
}

void ViewportCtl_OnMouseWheel(WPARAM wParam)
{
    /* Тут типа зум надо бы сделать, но хз
    int z_delta = GET_WHEEL_DELTA_WPARAM(wParam); */
}

void ViewportCtl_OnLButtonDown(MOUSEEVENT mEvt)
{
    bViewDrag = bViewDragKey;
    
    if (g_tool.onlbuttondown != NULL)
    {
      g_tool.onlbuttondown(mEvt);
    }
}

void ViewportCtl_OnLButtonUp(MOUSEEVENT mEvt)
{
    bViewDrag = FALSE;
    if (g_tool.onlbuttonup != NULL)
    {
      g_tool.onlbuttonup(mEvt);
    }
}

void ViewportCtl_OnMouseMove(MOUSEEVENT mEvt)
{
    if (bViewDrag)
    {
        dragPrev.x = LOWORD(mEvt.lParam);
        dragPrev.y = HIWORD(mEvt.lParam);
    }
    
    if (g_tool.onmousemove)
    {
      g_tool.onmousemove(mEvt);
    }
}

LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MOUSEEVENT mevt;
    mevt.hwnd = hwnd;
    mevt.lParam = lParam;

    switch(msg) {
    case WM_CREATE:         ViewportCtl_OnCreate();                     break;
    case WM_PAINT:          ViewportCtl_OnPaint(hwnd);                  break;
    case WM_KEYDOWN:        ViewportCtl_OnKeyDown(hwnd, wParam,
                                                  lParam);              break;
    case WM_MOUSEWHEEL:     ViewportCtl_OnMouseWheel(wParam);           break;
    case WM_LBUTTONDOWN:    ViewportCtl_OnLButtonDown(mevt);            break;
    case WM_LBUTTONUP:      ViewportCtl_OnLButtonUp(mevt);              break;
    case WM_MOUSEMOVE:      ViewportCtl_OnMouseMove(mevt);              break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

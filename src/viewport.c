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
#include "commontypes.h"

const WCHAR szViewportWndClass[] = L"Win32Class_Viewport";

struct _Viewport {
  Document* document;
  HWND hwnd; 
  int canvasX;
  int canvasY;
  BOOL bViewDrag;
};

void Viewport_Invalidate(Viewport* viewport)
{
  InvalidateRect(Viewport_GetHWND(viewport), NULL, TRUE);
}

void Viewport_SetDocument(Viewport* viewport, Document* document)
{
  assert(viewport);
  if (!viewport)
    return;

  viewport->document = document;
}

LRESULT CALLBACK Viewport_WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL Viewport_Register(HINSTANCE hInstance)
{
  WNDCLASSEX wcex  = {0};
  wcex.cbSize      = sizeof(WNDCLASSEX);
  wcex.style       = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)Viewport_WndProc;
  wcex.hInstance   = hInstance;
  wcex.hCursor     = LoadCursor(NULL, IDC_ARROW);
  wcex.lpszClassName = szViewportWndClass;

  return RegisterClassEx(&wcex);
}

Document* Viewport_GetDocument(Viewport* viewport)
{
  return viewport->document;
}

void OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  /* Do nothing for now */
}

BOOL GdiBlitCanvas(HDC hdc, int x, int y, Canvas* canvas)
{
  Rect rc = Canvas_GetSize(canvas);
  size_t bufSize;
  HBITMAP hbitmap = CreateBitmap(rc.right, rc.bottom, 1,
      sizeof(uint32_t) * 8, Canvas_GetBuffer(canvas, &bufSize));
  HDC bitmapdc;

  bitmapdc = CreateCompatibleDC(hdc);
  SelectObject(bitmapdc, hbitmap);

  BOOL status = BitBlt(hdc, x, y, rc.right, rc.bottom, bitmapdc, 0, 0, SRCCOPY);

  if (!status)
  {
    MessageBox(NULL, L"BitBlt failed", NULL, MB_OK | MB_ICONERROR);
  }

  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  return TRUE;
}

static void OnPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  PAINTSTRUCT ps = {0};
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);

  Document* document = Viewport_GetDocument(viewport);
  Canvas* canvas = Document_GetCanvas(document);
  GdiBlitCanvas(hdc, 0, 0, canvas);

  Rect rc = Canvas_GetSize(canvas);

  MoveToEx(hdc, rc.right, 0, NULL);
  LineTo(hdc, rc.right, rc.bottom);
  LineTo(hdc, 0, rc.bottom);
  EndPaint(hwnd, &ps);
}

void OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  if (wParam == VK_SPACE)
  {
    viewport->bViewDrag = TRUE;
  }
}

void OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  if (wParam == VK_SPACE)
  {
    viewport->bViewDrag = FALSE;
  }
}

void OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  if (viewport->bViewDrag)
  {
    viewport->canvasX = LOWORD(lParam);
    viewport->canvasY = HIWORD(lParam);
  }
}

void ProceedToolEvent(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);
  if (!viewport)
    return;

  Tool* tool = GetSelectedTool();
  assert(tool);
  if (!tool)
    return;

  switch (message)
  {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
      Tool_ProcessEvent(tool, viewport, message, wParam, lParam);
  }
}

LRESULT CALLBACK Viewport_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  Viewport* viewport = NULL;
  if (message == WM_CREATE || message == WM_NCCREATE)
  {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
    viewport = (Viewport*)lpcs->lpCreateParams;
    viewport->hwnd = hwnd;
  }

  switch (message)
  {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
      ProceedToolEvent(hwnd, message, wParam, lParam);
  }

  switch (message) {
  case WM_CREATE:
    OnCreate(hwnd, wParam, lParam);
    return 0;
  case WM_PAINT:
    OnPaint(hwnd, wParam, lParam);
    return 0;
  case WM_KEYDOWN:
    OnKeyDown(hwnd, wParam, lParam);
    return 0;
  case WM_KEYUP:
    OnKeyUp(hwnd, wParam, lParam);
    return 0;
  case WM_MOUSEMOVE:
    OnMouseMove(hwnd, wParam, lParam);
    return 0;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

Viewport* Viewport_Create(HWND hwndParent, Document* document)
{
  Viewport* viewport = calloc(1, sizeof(Viewport));

  HWND hwnd = CreateWindowEx(0, szViewportWndClass, L"", WS_OVERLAPPEDWINDOW |
      WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, hwndParent, NULL,
      (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE),
      (LPVOID)viewport);
  assert(hwnd);

  viewport->document = document;
  viewport->hwnd = hwnd;

  return viewport;
}

HWND Viewport_GetHWND(Viewport* viewport)
{
  return viewport->hwnd;
}

#include "precomp.h"
#include <windowsx.h>

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
#include "resource.h"

#include <strsafe.h>

#define IDM_VIEWPORTSETTINGS 100

Viewport g_viewport;
Tool g_tool;

struct _ViewportSettings {
  BOOL doubleBuffered;
  BOOL bkgndExcludeCanvas;
  BOOL showDebugInfo;

  enum {
    BKGND_RECTANGLES,
    BKGND_REGION
  } backgroundMethod;
};

struct _ViewportSettings g_viewportSettings = {
  .doubleBuffered = TRUE,
  .bkgndExcludeCanvas = TRUE,
  .showDebugInfo = TRUE,
  .backgroundMethod = BKGND_REGION
};

struct _CheckerConfig {
  int size;
  BOOL invalidate;
  HBRUSH hSolid1;
  HBRUSH hSolid2;
  HBRUSH hbrush;
};

struct _CheckerConfig g_checker = {
  .size = 16,
  .invalidate = TRUE
};

INT_PTR CALLBACK ViewportSettingsDlgProc(HWND hwndDlg, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      /* Restore check states */
      Button_SetCheck(GetDlgItem(hwndDlg, IDC_DOUBLEBUFFER),
          g_viewportSettings.doubleBuffered);
      Button_SetCheck(GetDlgItem(hwndDlg, IDC_BKGNDEXCLUDECANVAS),
          g_viewportSettings.bkgndExcludeCanvas);
      Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHOWDEBUG),
          g_viewportSettings.showDebugInfo);

      switch (g_viewportSettings.backgroundMethod)
      {
        case BKGND_RECTANGLES:
          Button_SetCheck(GetDlgItem(hwndDlg, IDC_BKGNDFILLRECT), TRUE);
          break;
        case BKGND_REGION:
          Button_SetCheck(GetDlgItem(hwndDlg, IDC_BKGNDREGION), TRUE);
          break;
      }

      /* Enable/disable sub-options */
      Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDFILLRECT),
          g_viewportSettings.bkgndExcludeCanvas);
      Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDREGION),
          g_viewportSettings.bkgndExcludeCanvas);

      return TRUE;

    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
        switch (LOWORD(wParam))
        {
          case IDC_DOUBLEBUFFER:
            g_viewportSettings.doubleBuffered = Button_GetCheck((HWND)lParam);
            break;
          case IDC_BKGNDEXCLUDECANVAS:
            {
              BOOL fStatus = Button_GetCheck((HWND)lParam);
              g_viewportSettings.bkgndExcludeCanvas = fStatus;

              Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDFILLRECT), fStatus);
              Button_Enable(GetDlgItem(hwndDlg, IDC_BKGNDREGION), fStatus);
            }
            break;
          case IDC_SHOWDEBUG:
            g_viewportSettings.showDebugInfo = Button_GetCheck((HWND)lParam);
            break;
          case IDC_BKGNDFILLRECT:
            g_viewportSettings.backgroundMethod = BKGND_RECTANGLES;
            break;
          case IDC_BKGNDREGION:
            g_viewportSettings.backgroundMethod = BKGND_REGION;
            break;
          case IDOK:
          case IDCANCEL:
            EndDialog(hwndDlg, wParam);
        }
      }

      Viewport_Invalidate();
      return TRUE;
  }

  return FALSE;
}

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
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
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

  g_checker.hSolid1 = CreateSolidBrush(RGB(0xCC, 0xCC, 0xCC));
  g_checker.hSolid2 = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
}

BOOL gdi_blit_canvas(HDC hdc, int x, int y, Canvas* canvas)
{
  assert(canvas);
  if (!canvas)
    return FALSE;

  HDC bitmapdc = CreateCompatibleDC(hdc);

  unsigned char* premul = malloc(canvas->buffer_size);
  ZeroMemory(premul, canvas->buffer_size);
  memcpy(premul, canvas->buffer, canvas->buffer_size);

  for (size_t y = 0; y < (size_t)canvas->height; y++)
  {
    for (size_t x = 0; x < (size_t)canvas->width; x++)
    {
      unsigned char *pixel= premul + (x + y * canvas->width) * 4;

      float factor = pixel[3] / 255.f;
      pixel[0] *= factor;
      pixel[1] *= factor;
      pixel[2] *= factor;
    }
  }

  HBITMAP hbitmap = CreateBitmap(canvas->width, canvas->height, 1,
      sizeof(uint32_t) * 8, premul);


  SelectObject(bitmapdc, hbitmap);

  BLENDFUNCTION blendFunc = {
    .BlendOp = AC_SRC_OVER,
    .BlendFlags = 0,
    .SourceConstantAlpha = 0xFF,
    .AlphaFormat = AC_SRC_ALPHA
  };

  AlphaBlend(hdc, x, y, canvas->width, canvas->height, bitmapdc, 0, 0,
      canvas->width, canvas->height, blendFunc);

  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  free(premul);
  return TRUE;
}

HBRUSH GetCheckerBrush(HDC hdc)
{
  if (g_checker.invalidate)
  {

    HBITMAP hPatBmp = CreateCompatibleBitmap(hdc, g_checker.size,
        g_checker.size);
    HDC hPatDC = CreateCompatibleDC(hdc);
    SelectObject(hPatDC, hPatBmp);

    RECT rcBack = {0, 0, g_checker.size, g_checker.size};
    FillRect(hPatDC, &rcBack, g_checker.hSolid2);

    RECT rcLump1 = {0, 0, g_checker.size/2, g_checker.size/2};
    FillRect(hPatDC, &rcLump1, g_checker.hSolid1);

    RECT rcLump2 = {g_checker.size/2, g_checker.size/2, g_checker.size,
        g_checker.size};
    FillRect(hPatDC, &rcLump2, g_checker.hSolid1);

    HBRUSH hCheckerBrush = CreatePatternBrush(hPatBmp);
    assert(hCheckerBrush);
    g_checker.hbrush = hCheckerBrush;

    g_checker.invalidate = FALSE;
  }

  return g_checker.hbrush;
}

static void DrawCheckerTexture(HDC hdc, int width, int height)
{
  RECT rc = {0, 0, width, height};
  HBRUSH hbrChecker = GetCheckerBrush(hdc);
  FillRect(hdc, &rc, hbrChecker);
}

void Viewport_OnPaint(HWND hwnd)
{
  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  HDC hdcTarget = hdc;

  RECT clientRc;
  HDC hdcBack;
  HBITMAP hbmBack;
  HGDIOBJ hOldObjBack;

  GetClientRect(hwnd, &clientRc);

  /* Using double buffering to avoid canvas flicker */
  if (g_viewportSettings.doubleBuffered) {

    /* Create back context and buffet, set it */
    hdcBack = CreateCompatibleDC(hdc);
    hbmBack = CreateCompatibleBitmap(hdc, clientRc.right, clientRc.bottom);
    hOldObjBack = SelectObject(hdcBack, hbmBack);

    /* Draw window background */
    HBRUSH hbr = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
    FillRect(hdcBack, &clientRc, hbr);

    /* Set backbuffer as draw target */
    hdcTarget = hdcBack;
  }

  /*Use hdcTarget for actual drawing */
  if (g_viewport.document)
  {
    DrawCheckerTexture(
        hdcTarget,
        g_viewport.document->canvas->width,
        g_viewport.document->canvas->height);

    /* Copy canvas to viewport */
    gdi_blit_canvas(hdcTarget, 0, 0, g_viewport.document->canvas);

    /* Draw canvas frame */
    MoveToEx(hdcTarget, g_viewport.document->canvas->width, 0, NULL);
    LineTo(hdcTarget, g_viewport.document->canvas->width,
        g_viewport.document->canvas->height);
    LineTo(hdcTarget, 0, g_viewport.document->canvas->height);
  }

  if (g_viewportSettings.showDebugInfo)
  {
    RECT textRc = clientRc;
    WCHAR szFormat[256] = L"Debug shown\n"
      L"Double Buffer: %s\n"
      L"Background exclude canvas: %s\n"
      L"Background method: %s\n";
    WCHAR szDebugString[256];
    StringCchPrintf(szDebugString, 256, szFormat,
        g_viewportSettings.doubleBuffered ? L"ON" : L"OFF",
        g_viewportSettings.bkgndExcludeCanvas ? L"ON" : L"OFF",
        g_viewportSettings.bkgndExcludeCanvas ? g_viewportSettings.backgroundMethod ? L"GDI region" : L"FillRect" : L"N/A");
    int len = wcslen(szDebugString);
    DrawText(hdcTarget, szDebugString, len, &clientRc, DT_BOTTOM | DT_RIGHT);
  }

  if (g_viewportSettings.doubleBuffered) {
    /* Copy backbuffer to window */
    BitBlt(hdc, 0, 0, clientRc.right, clientRc.bottom, hdcBack, 0, 0, SRCCOPY);

    /* Release and free backbuffer resources */
    SelectObject(hdcBack, hOldObjBack);
    DeleteObject(hbmBack);
    DeleteDC(hdcBack);
  }

  EndPaint(hwnd, &ps);
}

void Viewport_OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (wParam == VK_SPACE) {
    g_viewport.view_dragging = TRUE;
  }

  DefWindowProc(hwnd, WM_KEYDOWN, wParam, lParam);
}

void Viewport_OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (wParam == VK_SPACE) {
    g_viewport.view_dragging = FALSE;
  }

  DefWindowProc(hwnd, WM_KEYUP, wParam, lParam);
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

void Viewport_OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  if (LOWORD(wParam) == IDM_VIEWPORTSETTINGS)
  {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_VIEWPORTSETTINGS),
        hwnd, (DLGPROC)ViewportSettingsDlgProc);
  }
}

BOOL Viewport_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
  if (g_viewportSettings.doubleBuffered)
    /* Avoid pass to user32 */
    return TRUE;

  RECT clientRc;
  GetClientRect(hwnd, &clientRc);

  if (g_viewport.document && g_viewportSettings.bkgndExcludeCanvas)
  {
    RECT canvasRc;
    canvasRc.right = g_viewport.document->canvas->width;
    canvasRc.bottom = g_viewport.document->canvas->height;

    HBRUSH hbr = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);

    switch (g_viewportSettings.backgroundMethod)
    {
      case BKGND_RECTANGLES:
        {
          RECT rc1 = {0}, rc2 = {0};

          rc1.left = canvasRc.right + 1;
          rc1.bottom = canvasRc.bottom + 1;
          rc1.right = clientRc.right;

          rc2.top = canvasRc.bottom + 1;
          rc2.bottom = clientRc.bottom;
          rc2.right = clientRc.right;

          FillRect(hdc, &rc1, hbr);
          FillRect(hdc, &rc2, hbr);
        }
        break;
      case BKGND_REGION:
        {
          int iSavedState = SaveDC(hdc);
          SelectClipRgn(hdc, NULL);
          ExcludeClipRect(hdc, 0, 0, canvasRc.right + 1, canvasRc.bottom + 1);
          FillRect(hdc, &clientRc, hbr);
          RestoreDC(hdc, iSavedState);
        }
        break;
    }

    return TRUE;
  }

  return FALSE;
}

void Viewport_OnContextMenu(HWND hwnd, int x, int y)
{
  x = x < 0 ? 0 : x;
  y = y < 0 ? 0 : y;

  HMENU hMenu = CreatePopupMenu();
  InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_VIEWPORTSETTINGS,
      L"Settings");
  TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, hwnd, NULL);

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
    Viewport_Invalidate();
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
    DefWindowProc(hwnd, msg, wParam, lParam);
    break;
  case WM_MOUSEMOVE:
    Viewport_OnMouseMove(mevt);
    break;
  case WM_COMMAND:
    Viewport_OnCommand(hwnd, wParam, lParam);
    break;
  case WM_CONTEXTMENU:
    Viewport_OnContextMenu((HWND)wParam, GET_X_LPARAM(lParam),
        GET_Y_LPARAM(lParam));
    break;
  case WM_ERASEBKGND:
    return Viewport_OnEraseBkgnd(hwnd, (HDC)wParam);
    break;
  case WM_SIZE:
    Viewport_Invalidate();
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

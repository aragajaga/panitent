#include "precomp.h"
#include <windowsx.h>

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
#include "checker.h"

#include <strsafe.h>

#define IDM_VIEWPORTSETTINGS 100

Tool g_tool;

const WCHAR szViewportClass[] = L"Win32Class_Viewport";

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
  .showDebugInfo = FALSE,
  .backgroundMethod = BKGND_REGION
};

LRESULT CALLBACK Viewport_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam);
void Viewport_OnLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam);
void Viewport_OnLButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam);
void Viewport_OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam);
void Viewport_OnPaint(HWND hWnd);

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

      Viewport_Invalidate(Panitent_GetActiveViewport());
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

void Viewport_Invalidate(Viewport *viewport)
{
  InvalidateRect(viewport->hwnd, NULL, TRUE);
}

void Viewport_SetDocument(Viewport* viewport, Document* document)
{
  assert(viewport);
  assert(document);

  viewport->document = document;

  viewport->offset.x = 0;
  viewport->offset.y = 0;
  viewport->scale = 1.f;

  Viewport_Invalidate(viewport);
}

Document* Viewport_GetDocument(Viewport* viewport)
{
  return viewport->document;
}

BOOL Viewport_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex  = {0};
  wcex.cbSize      = sizeof(WNDCLASSEX);
  wcex.style       = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)Viewport_WndProc;
  wcex.hInstance   = hInstance;
  wcex.hCursor     = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = szViewportClass;

  return RegisterClassEx(&wcex);
}

void Viewport_OnCreate(HWND hwnd, LPCREATESTRUCT lpcs)
{
  UNREFERENCED_PARAMETER(lpcs);

  Viewport *viewport = calloc(1, sizeof(Viewport));
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)viewport);
  viewport->hwnd = hwnd;

  viewport->offset.x = 0;
  viewport->offset.y = 0;
  viewport->scale = 1.f;

  CheckerConfig checkerCfg;
  checkerCfg.tileSize = 16;
  checkerCfg.color[0] = 0x00CCCCCC;
  checkerCfg.color[1] = 0x00FFFFFF;

  HDC hdc = GetDC(hwnd);
  viewport->hbrChecker = CreateChecker(&checkerCfg, hdc);
  ReleaseDC(hwnd, hdc);
}

BOOL gdi_blit_canvas(HDC hdc, int x, int y, int x1, int y1, Canvas* canvas)
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

  AlphaBlend(hdc, x, y, x1, y1, bitmapdc, 0, 0,
      canvas->width, canvas->height, blendFunc);

  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  free(premul);
  return TRUE;
}

void Viewport_ClientToCanvas(Viewport* viewport, int x, int y, LPPOINT lpPt)
{
  Document* document = Viewport_GetDocument(viewport);
  assert(document);
  if (!document)
    return;

  Canvas* canvas = Document_GetCanvas(document);
  assert(canvas);
  if (!canvas)
    return;

  RECT clientRc = {0};
  GetClientRect(viewport->hwnd, &clientRc);

  /* TODO: Try to get rid of intermediate negative value. Juggle around with
     'x' and 'offset' positions. */
  lpPt->x = (x - (clientRc.right - canvas->width * viewport->scale) / 2 -
      viewport->offset.x) / viewport->scale;
  lpPt->y = (y - (clientRc.bottom - canvas->height * viewport->scale) / 2 -
      viewport->offset.y) / viewport->scale;
}

void Viewport_OnPaint(HWND hwnd)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  HDC hdcTarget = hdc;

  RECT clientRc;
  HDC hdcBack = NULL;
  HBITMAP hbmBack = NULL;
  HGDIOBJ hOldObjBack = NULL;

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


  Document* document = Viewport_GetDocument(viewport);
  if (document)
  {
    Canvas* canvas = Document_GetCanvas(document);
    assert(canvas);

    RECT renderRc = {
      .left = ((clientRc.right - canvas->width * viewport->scale) / 2
          + viewport->offset.x),
      .top = ((clientRc.bottom - canvas->height * viewport->scale) / 2
          + viewport->offset.y),
      .right = (canvas->width * viewport->scale),
      .bottom = (canvas->height * viewport->scale)
    };

    /* Use hdcTarget for actual drawing */
    if (viewport->document)
    {
      FillRect(hdcTarget, &renderRc, viewport->hbrChecker);

      /* Copy canvas to viewport */
      gdi_blit_canvas(hdcTarget,
          max(0, renderRc.left), max(0, renderRc.top),
          min(clientRc.right, renderRc.right),
          min(clientRc.bottom, renderRc.bottom),
          viewport->document->canvas);

      /* Draw canvas frame */
      RECT frameRc = {
        max(0, renderRc.left), max(0, renderRc.top),
        min(clientRc.right, renderRc.left + renderRc.right),
        min(clientRc.bottom, renderRc.top + renderRc.bottom)
      };
      FrameRect(hdcTarget, &frameRc, GetStockObject(BLACK_BRUSH));
    }

    if (g_viewportSettings.showDebugInfo)
    {

      /* Doesn't help */
      int state = SaveDC(hdcTarget);

      HGDIOBJ hOldObj_ = NULL;
      COLORREF prevTextColor = GetTextColor(hdcTarget);
      int prevBkMode = GetBkMode(hdcTarget);
      HFONT hFont = GetGuiFont();

      SetBkMode(hdcTarget, TRANSPARENT);

      HPEN redPen = CreatePen(PS_DASH, 1, RGB(255, 0, 0));
      hOldObj_ = SelectObject(hdcTarget, redPen);
      MoveToEx(hdcTarget, 0, clientRc.bottom / 2, NULL);
      LineTo(hdcTarget, clientRc.right, clientRc.bottom / 2);
      MoveToEx(hdcTarget, clientRc.right / 2, 0, NULL);
      LineTo(hdcTarget, clientRc.right / 2, clientRc.bottom);
      SelectObject(hdcTarget, hOldObj_);
      DeleteObject(redPen);

      hOldObj_ = SelectObject(hdcTarget, hFont);
      SetTextColor(hdcTarget, RGB(255, 0, 0));
      TextOut(hdcTarget, clientRc.right / 2 + 4, clientRc.bottom / 2,
          L"VIEWPORT CENTER", 15);
      SelectObject(hdcTarget, hOldObj_);

      HPEN greenPen = CreatePen(PS_DASH, 1, RGB(0, 255, 0));
      hOldObj_ = SelectObject(hdcTarget, greenPen);
      MoveToEx(hdcTarget, 0, clientRc.bottom / 2 + viewport->offset.y, NULL);
      LineTo(hdcTarget, clientRc.right, clientRc.bottom / 2 +
          viewport->offset.y);
      MoveToEx(hdcTarget, clientRc.right / 2 + viewport->offset.x, 0, NULL);
      LineTo(hdcTarget, clientRc.right / 2 + viewport->offset.x,
          clientRc.bottom);
      SelectObject(hdcTarget, hOldObj_);
      DeleteObject(greenPen);

      hOldObj_ = SelectObject(hdcTarget, hFont);
      WCHAR szOffset[80] = L"";
      StringCchPrintf(szOffset, 80, L"x: %d, y: %d", viewport->offset.x,
          viewport->offset.y);
      int lenLabel = wcslen(szOffset);
      SetTextColor(hdcTarget, RGB(0, 255, 0));
      TextOut(hdcTarget, clientRc.right / 2 + viewport->offset.x + 4,
          clientRc.bottom / 2 + viewport->offset.y, szOffset, lenLabel);
      SelectObject(hdcTarget, hOldObj_);

      HPEN bluePen = CreatePen(PS_DASH, 1, RGB(0, 0, 255));
      hOldObj_ = SelectObject(hdcTarget, bluePen);
      MoveToEx(hdcTarget, clientRc.right / 2, clientRc.bottom / 2, NULL);
      LineTo(hdcTarget, clientRc.right / 2 + viewport->offset.x,
          clientRc.bottom / 2 + viewport->offset.y);
      SelectObject(hdcTarget, hOldObj_);
      DeleteObject(bluePen);

      SetTextColor(hdcTarget, prevTextColor);
      SetBkMode(hdcTarget, prevBkMode);

      RestoreDC(hdc, state);

      RECT textRc = clientRc;
      WCHAR szFormat[256] = L"Debug shown\n"
        L"Double Buffer: %s\n"
        L"Background exclude canvas: %s\n"
        L"Background method: %s\n"
        L"Offset x: %d, y: %d\n"
        L"Scale: %f\n"
        L"Document set: %s\n"
        L"Document dimensions width: %d, height %d";
      WCHAR szDebugString[256];
      StringCchPrintf(szDebugString, 256, szFormat,
          g_viewportSettings.doubleBuffered ? L"ON" : L"OFF",
          g_viewportSettings.bkgndExcludeCanvas ? L"ON" : L"OFF",
          g_viewportSettings.bkgndExcludeCanvas ?
              g_viewportSettings.backgroundMethod ?
                  L"GDI region" : L"FillRect" : L"N/A",
          viewport->offset.x, viewport->offset.y,
          viewport->scale,
          viewport->document ? L"TRUE" : L"FALSE",
          viewport->document ? viewport->document->canvas->width : 0,
          viewport->document ? viewport->document->canvas->height : 0);

      int len = wcslen(szDebugString);

      HGDIOBJ hOldFont = SelectObject(hdcTarget, hFont);
      DrawText(hdcTarget, szDebugString, len, &clientRc, DT_BOTTOM | DT_RIGHT);
      SelectObject(hdcTarget, hOldFont);
    }

    if (g_viewportSettings.doubleBuffered) {
      /* Copy backbuffer to window */
      BitBlt(hdc, 0, 0, clientRc.right, clientRc.bottom, hdcBack, 0, 0, SRCCOPY);

      /* Release and free backbuffer resources */
      SelectObject(hdcBack, hOldObjBack);
      DeleteObject(hbmBack);
      DeleteDC(hdcBack);
    }
  }

  EndPaint(hwnd, &ps);
}

void Viewport_OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  if (wParam == VK_SPACE) {
    if (!viewport->fDrag)
    {
      POINT ptDelta;
      GetCursorPos(&ptDelta);
      ScreenToClient(hwnd, &ptDelta);

      viewport->drag.x = ptDelta.x;
      viewport->drag.y = ptDelta.y;

      viewport->fDrag = TRUE;
    }
  }

  DefWindowProc(hwnd, WM_KEYDOWN, wParam, lParam);
}

void Viewport_OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  if (wParam == VK_SPACE) {
    viewport->fDrag = FALSE;
  }

  DefWindowProc(hwnd, WM_KEYUP, wParam, lParam);
}

void Viewport_OnMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
  int distance = GET_WHEEL_DELTA_WPARAM(wParam);

  if (fwKeys & MK_CONTROL)
  {
    RECT clientRc = {0};
    GetClientRect(hwnd, &clientRc);

    viewport->scale = max(0.1f, viewport->scale + distance / (float)(120 * 16.f));

    Document* document = Viewport_GetDocument(viewport);
    Canvas* canvas = Document_GetCanvas(document);

    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    viewport->offset.x = -((x - clientRc.right / 2 + viewport->offset.x) /
        (canvas->width * viewport->scale / 2));
    viewport->offset.y = -((y - clientRc.bottom/ 2 + viewport->offset.y) /
        (canvas->height * viewport->scale / 2));
  }
  else if (GetKeyState(VK_MENU) & 0x8000)
  {
    viewport->offset.x += distance / 2;
  }
  else {
    viewport->offset.y += distance / 2;
  }

  Viewport_Invalidate(viewport);
}

void Viewport_OnLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  /* Receive keyboard messages */
  SetFocus(hwnd);

  if (g_tool.onlbuttondown != NULL)
  {
    POINT ptCanvas;
    Viewport_ClientToCanvas(viewport, x, y, &ptCanvas);

    g_tool.onlbuttondown(hwnd, wParam, MAKELPARAM(ptCanvas.x, ptCanvas.y));
  }
}

void Viewport_OnLButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  if (g_tool.onlbuttonup != NULL)
  {
    RECT clientRc;
    GetClientRect(hwnd, &clientRc);

    POINT ptCanvas;
    Viewport_ClientToCanvas(viewport, x, y, &ptCanvas);

    g_tool.onlbuttonup(hwnd, wParam, MAKELPARAM(ptCanvas.x, ptCanvas.y));
  }
}

void Viewport_OnRButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  if (g_tool.onrbuttonup != NULL)
  {
    POINT ptCanvas;
    Viewport_ClientToCanvas(viewport, x, y, &ptCanvas);

    g_tool.onrbuttonup(hwnd, wParam, MAKELPARAM(ptCanvas.x, ptCanvas.y));
  }
}

void Viewport_OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  if (viewport->fDrag)
  {
    viewport->offset.x = x - viewport->drag.x;
    viewport->offset.y = y - viewport->drag.y;

    InvalidateRect(hwnd, NULL, TRUE);

  }
  else if (g_tool.onmousemove)
  {
    POINT ptCanvas;
    Viewport_ClientToCanvas(viewport, x, y, &ptCanvas);

    g_tool.onmousemove(hwnd, wParam, MAKELPARAM(ptCanvas.x, ptCanvas.y));
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
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  if (g_viewportSettings.doubleBuffered)
    /* Avoid pass to user32 */
    return TRUE;

  RECT clientRc;
  GetClientRect(hwnd, &clientRc);

  if (viewport->document && g_viewportSettings.bkgndExcludeCanvas)
  {
    RECT canvasRc;
    canvasRc.right = viewport->document->canvas->width;
    canvasRc.bottom = viewport->document->canvas->height;

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

void Viewport_OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  int width = LOWORD(lParam);
  int height = HIWORD(lParam);

  if (viewport->document)
  {
    Viewport_Invalidate(viewport);
  }
}

LRESULT CALLBACK Viewport_WndProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
  switch (msg) {
  case WM_CREATE:
    Viewport_OnCreate(hwnd, (LPCREATESTRUCT)lParam);
    return 0;

  case WM_PAINT:
    Viewport_OnPaint(hwnd);
    return 0;

  case WM_KEYDOWN:
    Viewport_OnKeyDown(hwnd, wParam, lParam);
    return 0;

  case WM_KEYUP:
    Viewport_OnKeyUp(hwnd, wParam, lParam);
    return 0;

  case WM_MOUSEWHEEL:
    Viewport_OnMouseWheel(hwnd, wParam, lParam);
    return 0;

  case WM_LBUTTONDOWN:
    Viewport_OnLButtonDown(hwnd, wParam, lParam);
    return 0;

  case WM_LBUTTONUP:
    Viewport_OnLButtonUp(hwnd, wParam, lParam);
    return 0;

  case WM_RBUTTONUP:
    Viewport_OnRButtonUp(hwnd, wParam, lParam);
    /* Let system generate WM_CONTEXTMENU message*/
    break;

  case WM_MOUSEMOVE:
    Viewport_OnMouseMove(hwnd, wParam, lParam);
    return 0;

  case WM_COMMAND:
    Viewport_OnCommand(hwnd, wParam, lParam);
    return 0;

  case WM_CONTEXTMENU:
    Viewport_OnContextMenu((HWND)wParam, GET_X_LPARAM(lParam),
        GET_Y_LPARAM(lParam));
    return 0;

  case WM_ERASEBKGND:
    return Viewport_OnEraseBkgnd(hwnd, (HDC)wParam);

  case WM_SIZE:
    Viewport_OnSize(hwnd, wParam, lParam);
    return 0;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

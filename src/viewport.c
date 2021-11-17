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

extern inline void Viewport_ResetView(Viewport *viewport)
{
  viewport->ptOffset.x = 0;
  viewport->ptOffset.y = 0;
  viewport->fZoom = 1.f;
}

void Viewport_SetDocument(Viewport* viewport, Document* document)
{
  assert(viewport);
  assert(document);

  viewport->document = document;

  Viewport_ResetView(viewport);
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

  Viewport_ResetView(viewport);

  CheckerConfig checkerCfg;
  checkerCfg.tileSize = 16;
  checkerCfg.color[0] = 0x00CCCCCC;
  checkerCfg.color[1] = 0x00FFFFFF;

  HDC hdc = GetDC(hwnd);
  viewport->hbrChecker = CreateChecker(&checkerCfg, hdc);
  ReleaseDC(hwnd, hdc);
}

BOOL Viewport_BlitCanvas(HDC hDC, LPRECT prcView, Canvas *canvas)
{
  assert(canvas);
  if (!canvas)
    return FALSE;

  HDC hBmDC = CreateCompatibleDC(hDC);

  unsigned char *pData = malloc(canvas->buffer_size);
  ZeroMemory(pData, canvas->buffer_size);
  memcpy(pData, canvas->buffer, canvas->buffer_size);

  /* Premultiply alpha */
  for (size_t y = 0; y < (size_t) canvas->height; y++)
  {
    for (size_t x = 0; x < (size_t) canvas->width; x++)
    {
      unsigned char *pixel = pData + (x + y * canvas->width) * 4;

      float factor = pixel[3] / 255.f;
      pixel[0] *= factor;
      pixel[1] *= factor;
      pixel[2] *= factor;
    }
  }

  HBITMAP hBitmap = CreateBitmap(canvas->width, canvas->height, 1,
      sizeof(uint32_t) * 8, pData);

  SelectObject(hBmDC, hBitmap);

  BLENDFUNCTION blendFunc = {
    .BlendOp = AC_SRC_OVER,
    .BlendFlags = 0,
    .SourceConstantAlpha = 0xFF,
    .AlphaFormat = AC_SRC_ALPHA
  };

  AlphaBlend(hDC,
      prcView->left,
      prcView->top,
      prcView->right - prcView->left,
      prcView->bottom - prcView->top,
      hBmDC, 0, 0,
      canvas->width, canvas->height, blendFunc);

  DeleteObject(hBitmap);
  DeleteDC(hBmDC);

  free(pData);
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
     'x' and .ptOffset' positions. */
  lpPt->x = (x - (clientRc.right - canvas->width * viewport->fZoom) / 2 -
      viewport->ptOffset.x) / viewport->fZoom;
  lpPt->y = (y - (clientRc.bottom - canvas->height * viewport->fZoom) / 2 -
      viewport->ptOffset.y) / viewport->fZoom;
}

static inline void Viewport_DrawWindowOrdinates(HDC hDC, Viewport *viewport, LPRECT lpRect)
{
  HPEN hPen;
  HGDIOBJ hOldObj;
  HFONT hFont;

  hFont = GetGuiFont();

  hPen = CreatePen(PS_DASH, 1, RGB(255, 0, 0));
  hOldObj = SelectObject(hDC, hPen);

  SetBkMode(hDC, TRANSPARENT);

  MoveToEx(hDC, 0, lpRect->bottom / 2, NULL);
  LineTo(hDC, lpRect->right, lpRect->bottom / 2);
  MoveToEx(hDC, lpRect->right / 2, 0, NULL);
  LineTo(hDC, lpRect->right / 2, lpRect->bottom);

  SelectObject(hDC, hOldObj);
  DeleteObject(hPen);

  hOldObj = SelectObject(hDC, hFont);

  SetTextColor(hDC, RGB(255, 0, 0));

  TextOut(hDC, lpRect->right / 2 + 4, lpRect->bottom / 2,
      L"VIEWPORT CENTER", 15);

  SelectObject(hDC, hOldObj);
}

static inline void Viewport_DrawCanvasOrdinates(HDC hDC, Viewport *viewport, LPRECT lpRect)
{
  HPEN hPen;
  HGDIOBJ hOldObj;
  LPPOINT lpptOffset;
  HFONT hFont;

  lpptOffset = &(viewport->ptOffset);

  hPen = CreatePen(PS_DASH, 1, RGB(0, 255, 0));
  hOldObj = SelectObject(hDC, hPen);
  hFont = GetGuiFont();

  SetBkMode(hDC, TRANSPARENT);

  MoveToEx(hDC, 0, lpRect->bottom / 2 + lpptOffset->y, NULL);
  LineTo(hDC, lpRect->right, lpRect->bottom / 2 + lpptOffset->y);
  MoveToEx(hDC, lpRect->right / 2 + lpptOffset->x, 0, NULL);
  LineTo(hDC, lpRect->right / 2 + lpptOffset->x, lpRect->bottom);

  SelectObject(hDC, hOldObj);
  DeleteObject(hPen);

  hOldObj = SelectObject(hDC, hFont);

  WCHAR szOffset[80] = L"";
  StringCchPrintf(szOffset, 80, L"x: %d, y: %d", viewport->ptOffset.x,
      viewport->ptOffset.y);

  int lenLabel = wcslen(szOffset);

  SetTextColor(hDC, RGB(0, 255, 0));

  TextOut(hDC, lpRect->right / 2 + lpptOffset->x + 4,
      lpRect->bottom / 2 + lpptOffset->y, szOffset, lenLabel);

  SelectObject(hDC, hOldObj);
}

static inline void Viewport_DrawOffsetTrace(HDC hDC, Viewport *viewport, LPRECT lpRect)
{
  HPEN hPen;
  HGDIOBJ hOldObj;
  LPPOINT lpptOffset;

  lpptOffset = &(viewport->ptOffset);

  hPen = CreatePen(PS_DASH, 1, RGB(0, 0, 255));
  hOldObj = SelectObject(hDC, hPen);

  SetBkMode(hDC, TRANSPARENT);

  MoveToEx(hDC, lpRect->right / 2, lpRect->bottom / 2, NULL);
  LineTo(hDC, lpRect->right / 2 + lpptOffset->x,
      lpRect->bottom / 2 + lpptOffset->y);

  SelectObject(hDC, hOldObj);
  DeleteObject(hPen);
}

static inline void Viewport_DrawDebugText(HDC hDC, Viewport *viewport, LPRECT lpRect)
{
  HFONT hFont;
  HGDIOBJ hOldObj;
  WCHAR szDebugString[256];
  WCHAR szFormat[256] = L"Debug shown\n"
    L"Double Buffer: %s\n"
    L"Background exclude canvas: %s\n"
    L"Background method: %s\n"
    L"Offset x: %d, y: %d\n"
    L"Scale: %f\n"
    L"Document set: %s\n"
    L"Document dimensions width: %d, height %d";

  hFont = GetGuiFont();

  StringCchPrintf(szDebugString, 256, szFormat,
      g_viewportSettings.doubleBuffered ? L"ON" : L"OFF",
      g_viewportSettings.bkgndExcludeCanvas ? L"ON" : L"OFF",
      g_viewportSettings.bkgndExcludeCanvas ?
          g_viewportSettings.backgroundMethod ?
              L"GDI region" : L"FillRect" : L"N/A",
      viewport->ptOffset.x, viewport->ptOffset.y,
      viewport->fZoom,
      viewport->document ? L"TRUE" : L"FALSE",
      viewport->document ? viewport->document->canvas->width : 0,
      viewport->document ? viewport->document->canvas->height : 0);

  int len = wcslen(szDebugString);

  SetTextColor(hDC, RGB(0, 0, 0));

  hOldObj = SelectObject(hDC, hFont);
  DrawText(hDC, szDebugString, len, lpRect, DT_BOTTOM | DT_RIGHT);
  SelectObject(hDC, hOldObj);
}

static inline void Viewport_DrawDebugOverlay(HDC hDC, Viewport *viewport, LPRECT lpRect)
{
  Viewport_DrawWindowOrdinates(hDC, viewport, lpRect);
  Viewport_DrawCanvasOrdinates(hDC, viewport, lpRect);
  Viewport_DrawOffsetTrace(hDC, viewport, lpRect);
  Viewport_DrawDebugText(hDC, viewport, lpRect);
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

    RECT renderRc;

    renderRc.left = ((clientRc.right - canvas->width * viewport->fZoom) / 2
          + viewport->ptOffset.x);
    renderRc.top = ((clientRc.bottom - canvas->height * viewport->fZoom) / 2
          + viewport->ptOffset.y);
    renderRc.right = renderRc.left + (canvas->width * viewport->fZoom);
    renderRc.bottom = renderRc.top + (canvas->height * viewport->fZoom);

    /* Use hdcTarget for actual drawing */
    if (viewport->document)
    {
      /* Draw transparency checkerboard */
      FillRect(hdcTarget, &renderRc, viewport->hbrChecker);

      /* Copy canvas to viewport */
      Viewport_BlitCanvas(hdcTarget, &renderRc, viewport->document->canvas);

      /* Draw canvas frame */
      RECT frameRc = {
        max(0, renderRc.left), max(0, renderRc.top),
        min(clientRc.right, renderRc.right),
        min(clientRc.bottom, renderRc.bottom)
      };
      FrameRect(hdcTarget, &frameRc, GetStockObject(BLACK_BRUSH));
    }

    if (g_viewportSettings.showDebugInfo)
      Viewport_DrawDebugOverlay(hdcTarget, viewport, &clientRc);

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

void Viewport_OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  assert(viewport);

  if (wParam == VK_SPACE) {
    if (!viewport->bDrag)
    {
      POINT ptCursor;
      GetCursorPos(&ptCursor);
      ScreenToClient(hWnd, &ptCursor);

      viewport->ptDrag.x = ptCursor.x;
      viewport->ptDrag.y = ptCursor.y;

      viewport->bDrag = TRUE;
    }
  }

  if (wParam == VK_F3)
  {
    g_viewportSettings.showDebugInfo = !g_viewportSettings.showDebugInfo;
    Viewport_Invalidate(viewport);
  }

  DefWindowProc(hWnd, WM_KEYDOWN, wParam, lParam);
}

void Viewport_OnKeyUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  Viewport* viewport = (Viewport*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  assert(viewport);

  if (wParam == VK_SPACE) {
    viewport->bDrag = FALSE;
  }

  DefWindowProc(hWnd, WM_KEYUP, wParam, lParam);
}

void Viewport_OnMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
  int iWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

  if (fwKeys & MK_CONTROL)
  {
    viewport->fZoom = max(0.1f, viewport->fZoom + iWheelDelta /
        (float) (WHEEL_DELTA * 16.f));
  }

  /*  Scroll canvas horizontally
   * 
   *  GetKeyState is okay because WPARAM doesn't provide ALT key modifier
   */
  else if (GetKeyState(VK_MENU) & 0x8000) {
    viewport->ptOffset.x += iWheelDelta / 2;
  }

  /* Scroll canvas vertically */
  else {
    viewport->ptOffset.y += iWheelDelta / 2;
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
  { POINT ptCanvas; Viewport_ClientToCanvas(viewport, x, y, &ptCanvas);

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

void Viewport_OnMButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
  assert(viewport);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  if (!viewport->bDrag)
  {
    POINT ptCursor;
    GetCursorPos(&ptCursor);
    ScreenToClient(hWnd, &ptCursor);

    viewport->ptDrag.x = ptCursor.x;
    viewport->ptDrag.y = ptCursor.y;

    viewport->bDrag = TRUE;
  }
}

void Viewport_OnMButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
  assert(viewport);

  viewport->bDrag = FALSE;
}

void Viewport_OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Viewport *viewport = (Viewport*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  assert(viewport);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  if (viewport->bDrag)
  {
    viewport->ptOffset.x += x - viewport->ptDrag.x;
    viewport->ptOffset.y += y - viewport->ptDrag.y;

    viewport->ptDrag.x = x;
    viewport->ptDrag.y = y;

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

  case WM_MBUTTONDOWN:
    Viewport_OnMButtonDown(hwnd, wParam, lParam);
    return 0;

  case WM_MBUTTONUP:
    Viewport_OnMButtonUp(hwnd, wParam, lParam);
    return 0;

  case WM_RBUTTONUP:
    Viewport_OnRButtonUp(hwnd, wParam, lParam);
    /* Let system generate WM_CONTEXTMENU message */
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

#include "precomp.h"

#include <windowsx.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "primitives_context.h"
#include "option_bar.h"
#include "tool.h"
#include "toolbox.h"
#include "viewport.h"
#include "canvas.h"
#include "debug.h"
#include "resource.h"
#include "color_context.h"
#include "palette.h"
#include "commontypes.h"

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
#include "crefptr.h"
#endif

const WCHAR szToolboxWndClass[] = L"Win32Class_Toolbox";

static BOOL fDraw = FALSE;
static Rect prevRc;

typedef struct _Toolbox {
  Tool** tools;
  unsigned int toolCount;
  HWND hwnd;
} Toolbox;

void Toolbox_AddTool(Toolbox* tbox, Tool* tool)
{
  tbox->tools[(size_t)tbox->toolCount] = tool;
  tbox->toolCount++;
}

void Toolbox_RemoveTool(Toolbox* tbox)
{
  if (tbox->toolCount) {
    tbox->toolCount--;
  }
}

HBITMAP img_layout;

Toolbox* Toolbox_New()
{
  Toolbox* tbox = calloc(1, sizeof(Toolbox));
  return tbox;
}

void Toolbox_Init(Toolbox* tbox)
{
  img_layout = (HBITMAP)LoadBitmap(GetModuleHandle(NULL),
      MAKEINTRESOURCE(IDB_TOOLS));

  tbox->tools      = calloc(sizeof(Tool*), 11);
  tbox->toolCount = 0;

  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_POINTER));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_PENCIL));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_CIRCLE));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_LINE));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_RECTANGLE));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_TEXT));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_FILL));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_PICKER));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_BREAKER));
  /*
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_PEN));
  Toolbox_AddTool(tbox, GetDefaultTool(E_TOOL_BRUSH));
  */
}

HTHEME hTheme = NULL;
int btnSize   = 24;
int btnOffset = 4;

void Toolbox_DrawSingleButton(HDC hdc, int x, int y)
{
  if (!hTheme) {
    HWND hButton = CreateWindowEx(0, L"BUTTON", L"", 0, 0, 0, 0, 0, NULL, NULL,
        GetModuleHandle(NULL), NULL);
    hTheme = OpenThemeData(hButton, L"BUTTON");
  }

  RECT rc = {x, y, x + btnSize, y + btnSize};
  DrawThemeBackgroundEx(hTheme, hdc, BP_PUSHBUTTON, PBS_NORMAL, &rc, NULL);
}

void Toolbox_DrawButtons(Toolbox* tbox, HDC hdc)
{
  BITMAP bitmap;
  HDC hdcMem;
  HGDIOBJ oldBitmap;

  hdcMem    = CreateCompatibleDC(hdc);
  oldBitmap = SelectObject(hdcMem, img_layout);
  GetObject(img_layout, sizeof(bitmap), &bitmap);

  RECT rcClient = {0};
  GetClientRect(tbox->hwnd, &rcClient);

  size_t extSize  = btnSize + btnOffset;
  size_t rowCount = rcClient.right / btnSize;

  if (rowCount < 1) {
    rowCount = 1;
  }

  for (size_t i = 0; i < tbox->toolCount; i++) {
    int x = btnOffset + (i % rowCount) * extSize;
    int y = btnOffset + (i / rowCount) * extSize;

    /* Rectangle(hdc, x, y, x+btnSize, y+btnSize); */
    Toolbox_DrawSingleButton(hdc, x, y);

    int iBmp         = Tool_GetImg(tbox->tools[(ptrdiff_t)i]);
    const int offset = 4;
    TransparentBlt(hdc,
                   x + offset,
                   y + offset,
                   16,
                   bitmap.bmHeight,
                   hdcMem,
                   16 * iBmp,
                   0,
                   16,
                   bitmap.bmHeight,
                   (UINT)0x00FF00FF);
  }

  SelectObject(hdcMem, oldBitmap);
  DeleteDC(hdcMem);
}

void OnLButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Toolbox* tbox = (Toolbox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  signed short x;
  signed short y;

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  int btnHot = btnSize + 5;

  if (x >= 0 && y >= 0 && x < btnHot * 2 &&
      y < (int)tbox->toolCount * btnHot) {
    size_t extSize = btnSize + btnOffset;

    RECT rcClient = {0};
    GetClientRect(hwnd, &rcClient);

    size_t rowCount = rcClient.right / extSize;
    if (rowCount < 1) {
      rowCount = 1;
    }

    unsigned int pressed =
        (y - btnOffset) / extSize * rowCount + (x - btnOffset) / extSize;

    if (tbox->toolCount > pressed) {
      switch (pressed) {
        case 1:
          SelectTool(GetDefaultTool(E_TOOL_PENCIL));
          break;
        case 2:
          SelectTool(GetDefaultTool(E_TOOL_CIRCLE));
          break;
        case 3:
          SelectTool(GetDefaultTool(E_TOOL_LINE));
          break;
        case 4:
          SelectTool(GetDefaultTool(E_TOOL_RECTANGLE));
          break;
        case 5:
          SelectTool(GetDefaultTool(E_TOOL_TEXT));
          break;
        case 6:
          SelectTool(GetDefaultTool(E_TOOL_FILL));
          break;
        case 7:
          SelectTool(GetDefaultTool(E_TOOL_PICKER));
          break;
        case 8:
          *((int *)0xFFFFFFFF) = 1;
          break;
        default:
          SelectTool(GetDefaultTool(E_TOOL_POINTER));
          break;
      }
    }
  }
}

HWND Toolbox_GetHWND(Toolbox* tbox)
{
  return tbox->hwnd;
}

static void OnPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  Toolbox* tbox = (Toolbox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  PAINTSTRUCT ps;
  HDC hdc;
  hdc = BeginPaint(tbox->hwnd, &ps);

  Toolbox_DrawButtons(tbox, hdc);

  EndPaint(Toolbox_GetHWND(tbox), &ps);
}

LRESULT CALLBACK Toolbox_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message) {
    case WM_CREATE:
      {
        LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;
        Toolbox* tbox     = (Toolbox*)cs->lpCreateParams;
        tbox->hwnd        = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)tbox);

        Toolbox_Init(tbox);
      }
      return 0;
    case WM_PAINT:
      OnPaint(hwnd, wParam, lParam);
      return 0;
    case WM_LBUTTONUP:
      OnLButtonUp(hwnd, wParam, lParam);
      return 0;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL Toolbox_Register(HINSTANCE hInstance)
{
  WNDCLASSEX wc    = {0};
  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = Toolbox_WndProc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = szToolboxWndClass;
  wc.hInstance     = hInstance;

  return RegisterClassEx(&wc);
}

void Toolbox_UnregisterClass()
{
  UnregisterClass(szToolboxWndClass, NULL);
}

HWND Toolbox_Create(HWND hwndParent)
{
  Toolbox* tbox = Toolbox_New();

  tbox->hwnd = CreateWindowEx(0, szToolboxWndClass, L"Toolbox", WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 0, 0, hwndParent, NULL,
      (HINSTANCE)GetWindowLongPtr(hwndParent, GWLP_HINSTANCE), (LPVOID)tbox);
  Toolbox_Init(tbox);

  return tbox->hwnd;
}

#include "precomp.h"

#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <windowsx.h>

#include "primitives_context.h"
#include "option_bar.h"
#include "tool.h"
#include "toolbox.h"
#include "viewport.h"
#include "brush.h"
#include "canvas.h"
#include "debug.h"
#include "resource.h"
#include "color_context.h"
#include "palette.h"
#include "history.h"
#include "panitent.h"
#include <strsafe.h>

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
#include "crefptr.h"
#endif

extern Tool g_tool;

LRESULT CALLBACK Toolbox_WndProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
void Toolbox_OnPaint(Toolbox*);
void Toolbox_OnMouseMove(HWND hwnd, LPARAM lParam);

void ToolPointer_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolPointer_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolPointer_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolPointer_Init();

void ToolPencil_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolPencil_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolPencil_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolPencil_Init();

void ToolCircle_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolCircle_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolCircle_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolCircle_Init();

void ToolLine_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolLine_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolLine_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolLine_Init();

void ToolRectangle_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolRectangle_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolRectangle_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolRectangle_Init();

void ToolText_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolText_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolText_OnLMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam);
void ToolText_Init();

void ToolFill_Init();
void ToolPicker_Init();
void ToolBrush_Init();

Tool g_tool_circle;
Tool g_tool_line;
Tool g_tool_pencil;
Tool g_tool_pointer;
Tool g_tool_rectangle;
Tool g_tool_text;
Tool g_tool_fill;
Tool g_tool_picker;
Tool g_tool_brush;

static BOOL fDraw = FALSE;
static POINT prev;

void Toolbox_AddTool(Toolbox* tbox, Tool tool)
{
  tbox->tools[(size_t)tbox->tool_count] = tool;
  tbox->tool_count++;
}

void Toolbox_RemoveTool(Toolbox* tbox)
{
  if (tbox->tool_count) {
    tbox->tool_count--;
  }
}

HBITMAP img_layout;

void ToolBrush_Init();
unsigned int g_uToolSelected;

void Toolbox_Init(Toolbox* tbox)
{
  ToolPointer_Init();
  ToolPencil_Init();
  ToolLine_Init();
  ToolCircle_Init();
  ToolRectangle_Init();
  ToolText_Init();
  ToolFill_Init();
  ToolPicker_Init();
  ToolBrush_Init();

  g_tool = g_tool_pointer;
  g_uToolSelected = 0;

  img_layout = (HBITMAP)LoadBitmap(GetModuleHandle(NULL),
      MAKEINTRESOURCE(IDB_TOOLS));

  tbox->tools      = calloc(sizeof(Tool), 16);
  tbox->tool_count = 0;

  Toolbox_AddTool(tbox, g_tool_pointer);
  Toolbox_AddTool(tbox, g_tool_pencil);
  Toolbox_AddTool(tbox, g_tool_circle);
  Toolbox_AddTool(tbox, g_tool_line);
  Toolbox_AddTool(tbox, g_tool_rectangle);
  Toolbox_AddTool(tbox, g_tool_text);
  Toolbox_AddTool(tbox, g_tool_fill);
  Toolbox_AddTool(tbox, g_tool_picker);
  Toolbox_AddTool(tbox, g_tool_brush);
}

HTHEME hTheme = NULL;
int btnSize   = 24;
int btnOffset = 4;

enum {
  NORMAL,
  PUSHED
};

void Toolbox_ButtonDraw(HDC hdc, int x, int y, unsigned int state)
{
  if (!hTheme) {
    HWND hButton = CreateWindowEx(0, L"BUTTON", L"",
        0, 0, 0, 0,
        0, NULL, NULL, GetModuleHandle(NULL), NULL);
    hTheme = OpenThemeData(hButton, L"BUTTON");
  }

  RECT rc = {x, y, x + btnSize, y + btnSize};

  if (hTheme)
  {
    int iStateId = PBS_NORMAL;
    if (state == PUSHED)
      iStateId = PBS_PRESSED;

    if (IsThemeBackgroundPartiallyTransparent(hTheme, BP_PUSHBUTTON, iStateId))
    {
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6387 )
#endif  /* _MSC_VER */

      DrawThemeParentBackground(NULL, hdc, NULL);

#ifdef _MSC_VER
#pragma warning ( pop )
#endif  /* _MSC_VER */
    }

    DrawThemeBackgroundEx(hTheme, hdc, BP_PUSHBUTTON, iStateId, &rc, NULL);
  }
  else {
    DWORD edge = EDGE_RAISED;
    if (state == PUSHED)
      edge = EDGE_SUNKEN;

    DrawEdge(hdc, &rc, edge, BF_RECT);
  }
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

  int extSize  = btnSize + btnOffset;
  int rowCount = rcClient.right / btnSize;

  if (rowCount < 1) {
    rowCount = 1;
  }

  for (unsigned int i = 0; i < tbox->tool_count; i++) {
    int x = btnOffset + (i % rowCount) * extSize;
    int y = btnOffset + (i / rowCount) * extSize;

    /* Rectangle(hdc, x, y, x+btnSize, y+btnSize); */

    unsigned int uState = NORMAL;
    if ((unsigned int)i == g_uToolSelected)
    {
      uState = PUSHED;
    }

    Toolbox_ButtonDraw(hdc, x, y, uState);

    int iBmp         = tbox->tools[(ptrdiff_t)i].img;
    const int offset = 4;
#ifdef TOOLBAR_32BPP_ICONS
    BLENDFUNCTION blendFunc = {
      .BlendOp = AC_SRC_OVER,
      .BlendFlags = 0,
      .SourceConstantAlpha = 0xFF,
      .AlphaFormat = AC_SRC_ALPHA
    };

    AlphaBlend(hdc,
        x + offset,
        y + offset,
        16,
        bitmap.bmHeight,
        hdcMem,
        16 * iBmp,
        0,
        16,
        bitmap.bmHeight,
        blendFunc);
#else
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
#endif
  }

  SelectObject(hdcMem, oldBitmap);
  DeleteDC(hdcMem);
}

void Toolbox_OnPaint(Toolbox* tbox)
{
  PAINTSTRUCT ps;
  HDC hdc;
  hdc = BeginPaint(tbox->hwnd, &ps);

  Toolbox_DrawButtons(tbox, hdc);

  EndPaint(tbox->hwnd, &ps);
}

void Toolbox_OnLButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  Toolbox* tbox = (Toolbox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  int btnHot = btnSize + 5;

  if (x >= 0 && y >= 0 && x < btnHot * 2 &&
      y < (int)tbox->tool_count * btnHot) {
    size_t extSize = (size_t)btnSize + (size_t)btnOffset;

    RECT rcClient = {0};
    GetClientRect(hwnd, &rcClient);

    size_t rowCount = rcClient.right / extSize;
    if (rowCount < 1) {
      rowCount = 1;
    }

    unsigned int pressed =
        (y - btnOffset) / (int)extSize * (int)rowCount + (x - btnOffset) / (int)extSize;

    g_uToolSelected = pressed;

    if (tbox->tool_count > pressed) {
      switch (pressed) {
      case 1:
        g_tool = g_tool_pencil;
        break;
      case 2:
        g_tool = g_tool_circle;
        break;
      case 3:
        g_tool = g_tool_line;
        break;
      case 4:
        g_tool = g_tool_rectangle;
        break;
      case 5:
        g_tool = g_tool_text;
        break;
      case 6:
        g_tool = g_tool_fill;
        break;
      case 7:
        g_tool = g_tool_picker;
        break;
      case 8:
        g_tool = g_tool_brush;
        break;
      default:
        g_tool = g_tool_pointer;
        break;
      }

#ifdef HAS_DISCORDSDK
      WCHAR szStatus[80] = L"";
      StringCchPrintf(szStatus, 80, L"Drawing with %s", g_tool.label);
      Discord_SetActivityStatus(g_panitent.discord, szStatus);
#endif /* HAS_DISCORDSDK */
    }
  }

  InvalidateRect(tbox->hwnd, NULL, TRUE);
}

LRESULT CALLBACK Toolbox_WndProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
  switch (msg) {
  case WM_CREATE:
  {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;

    Toolbox* tbox   = (Toolbox*)cs->lpCreateParams;
    tbox->hwnd        = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)tbox);

    Toolbox_Init(tbox);
  } break;
  case WM_PAINT:
  {
    Toolbox* tbox = (Toolbox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (tbox) {
      Toolbox_OnPaint(tbox);
    }
  } break;
  case WM_LBUTTONUP:
  {
    Toolbox_OnLButtonUp(hwnd, wParam, lParam);
  } break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

BOOL Toolbox_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  ZeroMemory(&wcex, sizeof(wcex));

  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = Toolbox_WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = TOOLBOX_WC;

  return RegisterClassEx(&wcex);
}

void Toolbox_UnregisterClass()
{
  UnregisterClass(TOOLBOX_WC, NULL);
}

void ToolPointer_Init()
{
  g_tool_pointer.label = L"Pointer";
  g_tool_pointer.img   = 0;
}

void ToolText_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  HDC viewportdc = GetDC(hViewport);
  HDC bitmapdc   = CreateCompatibleDC(viewportdc);

  signed short mouseX = GET_X_LPARAM(lParam);
  signed short mouseY = GET_Y_LPARAM(lParam);

  wchar_t textbuf[1024];
  size_t textLen = 0;
  GetWindowText(g_option_bar.textstring_handle,
                textbuf,
                sizeof(textbuf) / sizeof(wchar_t));
  textLen = wcslen(textbuf);

  HFONT hFont = CreateFont(24, 0, 0, 0, 600, FALSE, FALSE, FALSE,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH, L"Arial");
  SelectObject(bitmapdc, hFont);

  SIZE size = {0};
  GetTextExtentPoint32(bitmapdc, textbuf, (int)textLen, &size);
  RECT textrc = {0, 0, size.cx, size.cy};

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = size.cx;
  bmi.bmiHeader.biHeight = size.cy;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  uint32_t *buffer = NULL;

  HBITMAP hbitmap = CreateDIBSection(bitmapdc, &bmi, 0, (LPVOID*)&buffer, NULL, 0);
  assert(hbitmap != NULL);
  assert(buffer != NULL);

  HGDIOBJ hOldObj = SelectObject(bitmapdc, hbitmap);

  SetTextColor(bitmapdc, 0x00FFFFFF);
  SetBkColor(bitmapdc, 0x00000000);
  SetBkMode(bitmapdc, OPAQUE);

  DrawTextEx(bitmapdc, textbuf, -1, &textrc, 0, NULL);

  SelectObject(bitmapdc, hOldObj);

  for (int y = 0; y < size.cy; y++)
  {
    for (int x = 0; x < size.cx; x++)
    {
      uint32_t *pixel = buffer + (size_t)y * (size_t)size.cx + (size_t)x;
      *pixel = *pixel | 0xFF000000;
    }
  }

  Canvas_ColorStencilBits(viewport->document->canvas, buffer, mouseX, mouseY, size.cx, size.cy, g_color_context.fg_color);

  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolText_Init()
{
  g_tool_text.label       = L"Text";
  g_tool_text.img         = 6;
  g_tool_text.onlbuttonup = ToolText_OnLButtonUp;
}

void ToolPencil_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  long int x = GET_X_LPARAM(lParam);
  long int y = GET_Y_LPARAM(lParam);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(hViewport);
  prev.x = x;
  prev.y = y;
}

void ToolPencil_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(hViewport);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    Canvas* canvas = viewport->document->canvas;
    draw_line(canvas, prev.x, prev.y, x, y);

#ifdef PEN_OVERLAY
    ReleaseDC(hViewport, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolPencil_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;
    hdc = GetDC(hViewport);

    /* Draw overlay path */
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    /* Draw on canvas */
    Canvas* canvas = viewport->document->canvas;
    draw_line(canvas, prev.x, prev.y, x, y);

    prev.x = x;
    prev.y = y;

#ifdef PEN_OVERLAY
    ReleaseDC(hViewport, hdc);
#endif
  }
}

void ToolPencil_Init()
{
  g_tool_pencil.label         = L"Pencil";
  g_tool_pencil.img           = 1;
  g_tool_pencil.onlbuttonup   = ToolPencil_OnLButtonUp;
  g_tool_pencil.onlbuttondown = ToolPencil_OnLButtonDown;
  g_tool_pencil.onmousemove   = ToolPencil_OnMouseMove;
}

POINT circCenter;

void ToolCircle_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(hViewport);
  circCenter.x = x;
  circCenter.y = y;
}

void ToolCircle_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(hViewport);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    int radius;
    if (!float2int_s(&radius, sqrtf(powf((float)x - circCenter.x, 2.f) + powf((float)y - circCenter.y, 2.f))))
      return;

    Canvas* canvas = viewport->document->canvas;
    draw_circle(canvas, circCenter.x, circCenter.y, radius);

#ifdef PEN_OVERLAY
    ReleaseDC(hViewport, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolCircle_Init()
{
  g_tool_circle.label         = L"Circle";
  g_tool_circle.img           = 2;
  g_tool_circle.onlbuttonup   = ToolCircle_OnLButtonUp;
  g_tool_circle.onlbuttondown = ToolCircle_OnLButtonDown;
}

void ToolLine_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(hViewport);
  prev.x = x;
  prev.y = y;
}

void ToolLine_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  if (fDraw) {
    Canvas* canvas = viewport->document->canvas;
    draw_line(canvas, prev.x, prev.y, x, y);
  }
  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolLine_Init()
{
  g_tool_line.label         = L"Line";
  g_tool_line.img           = 3;
  g_tool_line.onlbuttonup   = ToolLine_OnLButtonUp;
  g_tool_line.onlbuttondown = ToolLine_OnLButtonDown;
}

void ToolRectangle_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  fDraw = TRUE;
  SetCapture(hViewport);
  prev.x = x;
  prev.y = y;

  History_StartDifferentiation(Panitent_GetActiveDocument());
}

void ToolRectangle_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  if (fDraw) {
    Canvas* canvas = viewport->document->canvas;
    draw_rectangle(canvas, prev.x, prev.y, x, y);
  }
  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolRectangle_Init()
{
  g_tool_rectangle.label         = L"Rectangle";
  g_tool_rectangle.img           = 4;
  g_tool_rectangle.onlbuttonup   = ToolRectangle_OnLButtonUp;
  g_tool_rectangle.onlbuttondown = ToolRectangle_OnLButtonDown;
}

#define CAPACITY_GROW 32

#define DECLARE_TYPED_QUEUE(T)                                                 \
struct _tqueue$$##T {                                                          \
  size_t capacity;                                                             \
  size_t length;                                                               \
  T* data;                                                                     \
};                                                                             \
                                                                               \
typedef struct _tqueue$$##T tqueue$$##T_t;                                     \
                                                                               \
tqueue$$##T_t* tqueue$$##T_new()                                               \
{                                                                              \
  tqueue$$##T_t* q = calloc(1, sizeof(tqueue##$$T_t));                         \
  assert(q);                                                                   \
                                                                               \
  return q;                                                                    \
}                                                                              \
                                                                               \
void tqueue$$##T_push(tqueue$$##T_t* q, T data)                                \
{                                                                              \
  assert(q);                                                                   \
  if (!q)                                                                      \
    return;                                                                    \
                                                                               \
  assert(q->data);                                                             \
  if (!q->data)                                                                \
    return;                                                                    \
                                                                               \
  if (q->capacity <= q->length)                                                \
  {                                                                            \
    T* pData = NULL;                                                           \
    pData = realloc(q->data, (q->capacity + CAPACITY_GROW) * sizeof(T));       \
    assert(pData && L"Unable to reallocate memory");                           \
    if (pData)                                                                 \
      q->data = pData;                                                         \
    else                                                                       \
      return;                                                                  \
    q->capacity += CAPACITY_GROW;                                              \
  }                                                                            \
                                                                               \
  q->data[q->length++] = data;                                                 \
}                                                                              \
                                                                               \
T tqueue$$##T_pop(tqueue$$##T_t* q)                                            \
{                                                                              \
  assert(q);                                                                   \
  assert(q->length);                                                           \
                                                                               \
  return q->data[--q->length];                                                 \
}                                                                              \
                                                                               \
void tqueue$$##T_delete(tqueue$$##T_t* q)                                      \
{                                                                              \
  assert(q);                                                                   \
  if (!q)                                                                      \
    return;                                                                    \
                                                                               \
  free(q);                                                                     \
}

#define tqueue_t(T) tqueue$$##T_t
#define tqueue_new(T) tqueue$$##T_new
#define tqueue_push(T) tqueue$$##T_push
#define tqueue_pop(T) tqueue$$##T_pop
#define tqueue_delete(T) tqueue$$##T_delete

DECLARE_TYPED_QUEUE(POINT)

typedef unsigned char byte_t;

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
struct _queue {
  size_t capacity;
  size_t length;
  size_t elSize;
  void* data;
};

typedef struct _queue queue_t;

queue_t* queue_new(size_t elSize)
{
  queue_t* q = malloc(sizeof(queue_t));
  q->elSize = elSize;
  q->length = 0;
  q->capacity = CAPACITY_GROW;
  q->data = malloc(q->elSize * CAPACITY_GROW);
  assert(q->data);

  return q;
}

void queue_push(queue_t* q, void* data)
{
  assert(q);
  if (!q)
    return;

  if (q->capacity <= q->length)
  {
    q->data = realloc(q->data, (q->capacity + CAPACITY_GROW) * q->elSize);
    q->capacity += CAPACITY_GROW;
  }

  memcpy((byte_t*)q->data + (q->length * q->elSize), data, q->elSize);
  q->length++;
}

crefptr_t* queue_pop(queue_t* q)
{
  assert(q);
  if (!q)
    return NULL;

  assert(q->length);
  if (!q->length)
    return NULL;

  /*
  TODO: Shrink unused memory
  if (q->capacity > q->length + CAPACITY_GROW) {
    size_t newcap = CAPACITY_GROW + ((q->length - 1) / CAPACITY_GROW) * CAPACITY_GROW;
    q->data = realloc(q->data, newcap * q->elSize);
    q->capacity = newcap;
  }
  */

  char* el = malloc(q->elSize);
  memcpy(el, (byte_t*)q->data + (q->length - 1) * q->elSize, q->elSize);
  crefptr_t* ptr = crefptr_new(el, NULL);
  q->length--;
  return ptr;
}

void queue_delete(queue_t* q)
{
  assert(q);
  if (!q)
    return;

  free(q->data);
  free(q);
}
#endif

void ToolPicker_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  uint32_t color = Canvas_GetPixel(viewport->document->canvas, x, y);
  SetForegroundColor(color);
}

void ToolPicker_OnRButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  uint32_t color = Canvas_GetPixel(viewport->document->canvas, x, y);
  SetBackgroundColor(color);
}

void ToolFill_DoFloodFill(Viewport* viewport, signed short x, signed short y,
    uint32_t newColor)
{
  assert(viewport);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  Canvas* canvas = viewport->document->canvas;
  if (!canvas)
    return;

  uint32_t oldColor = Canvas_GetPixel(canvas, x, y);
  POINT nextpt = {x, y};

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
  queue_t* q = queue_new(sizeof(POINT));
  queue_push(q, &nextpt);
#else
  tqueue_t(POINT)* q = tqueue_new(POINT)();
  tqueue_push(POINT)(q, nextpt);
#endif

  while (q->length)
  {
    LPPOINT ppt = NULL;

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
    crefptr_t* ptr = queue_pop(q);
    ppt = crefptr_get(ptr);
#else
    POINT pt = tqueue_pop(POINT)(q);
    ppt = &pt;
#endif

    if (Canvas_GetPixel(canvas, ppt->x + 1, ppt->y) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x + 1, ppt->y, newColor);
      nextpt.x = ppt->x + 1;
      nextpt.y = ppt->y;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    if (Canvas_GetPixel(canvas, ppt->x - 1, ppt->y) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x - 1, ppt->y, newColor);
      nextpt.x = ppt->x - 1;
      nextpt.y = ppt->y;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    if (Canvas_GetPixel(canvas, ppt->x, ppt->y + 1) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x, ppt->y + 1, newColor);
      nextpt.x = ppt->x;
      nextpt.y = ppt->y + 1;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    if (Canvas_GetPixel(canvas, ppt->x, ppt->y - 1) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x, ppt->y - 1, newColor);
      nextpt.x = ppt->x;
      nextpt.y = ppt->y - 1;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    Viewport_Invalidate(Panitent_GetActiveViewport());

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
    crefptr_deref(ptr);
#endif
  }

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
  queue_delete(q);
#else
  tqueue_delete(POINT)(q);
#endif
  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolFill_OnRButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  ToolFill_DoFloodFill(viewport, x, y, g_color_context.bg_color);
}

void ToolFill_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  ToolFill_DoFloodFill(viewport, x, y, g_color_context.fg_color);
}

void ToolFill_Init()
{
  g_tool_fill.label = L"Flood fill";
  g_tool_fill.img = 8;
  g_tool_fill.onlbuttonup = ToolFill_OnLButtonUp;
  g_tool_fill.onrbuttonup = ToolFill_OnRButtonUp;
}

void ToolPicker_Init()
{
  g_tool_picker.label = L"Color picker";
  g_tool_picker.img = 9;
  g_tool_picker.onlbuttonup = ToolPicker_OnLButtonUp;
  g_tool_picker.onrbuttonup = ToolPicker_OnRButtonUp;
}

Brush* g_pBrushDraw;

void ToolBrush_OnLButtonUp(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(hViewport);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());

  Brush_Delete(g_pBrushDraw);
}

void ToolBrush_OnLButtonDown(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(hViewport);
  prev.x = x;
  prev.y = y;

  g_pBrushDraw = BrushBuilder_Build(g_pBrush, g_brushSize);

  Brush_Draw(g_pBrushDraw, x, y, viewport->document->canvas,
      g_color_context.fg_color);
  Viewport_Invalidate(viewport);
}

void ToolBrush_OnMouseMove(HWND hViewport, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  Viewport* viewport = (Viewport*)GetWindowLongPtr(hViewport, GWLP_USERDATA);
  assert(viewport);

  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam);

  if (fDraw)
  {
    Brush_DrawTo(g_pBrushDraw, prev.x, prev.y, x, y, viewport->document->canvas,
        g_color_context.fg_color);

    prev.x = x;
    prev.y = y;

    Viewport_Invalidate(viewport);
  }
}

void ToolBrush_Init()
{
  g_tool_brush.label = L"Brush";
  g_tool_brush.img = 12;
  g_tool_brush.onlbuttondown = ToolBrush_OnLButtonDown;
  g_tool_brush.onlbuttonup = ToolBrush_OnLButtonUp;
  g_tool_brush.onmousemove = ToolBrush_OnMouseMove;
}

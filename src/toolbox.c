#include "precomp.h"

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
#include "brush.h"
#include "canvas.h"
#include "debug.h"
#include "resource.h"
#include "color_context.h"
#include "palette.h"
#include "history.h"
#include "panitent.h"

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
#include "crefptr.h"
#endif

extern Viewport g_viewport;

extern Tool g_tool;

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
    HWND hButton = CreateWindowEx(0,
                                  L"BUTTON",
                                  L"",
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  NULL,
                                  NULL,
                                  GetModuleHandle(NULL),
                                  NULL);
    hTheme       = OpenThemeData(hButton, L"BUTTON");
  }

  RECT rc = {x, y, x + btnSize, y + btnSize};

  if (hTheme)
  {
    int iStateId = PBS_NORMAL;
    if (state == PUSHED)
      iStateId = PBS_PRESSED;

    if (IsThemeBackgroundPartiallyTransparent(hTheme, BP_PUSHBUTTON, iStateId))
    {
      DrawThemeParentBackground(NULL, hdc, NULL);
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

  size_t extSize  = btnSize + btnOffset;
  size_t rowCount = rcClient.right / btnSize;

  if (rowCount < 1) {
    rowCount = 1;
  }

  for (size_t i = 0; i < tbox->tool_count; i++) {
    int x = btnOffset + (i % rowCount) * extSize;
    int y = btnOffset + (i / rowCount) * extSize;

    /* Rectangle(hdc, x, y, x+btnSize, y+btnSize); */

    unsigned int uState = NORMAL;
    if (i == g_uToolSelected)
    {
      uState = PUSHED;
    }

    Toolbox_ButtonDraw(hdc, x, y, uState);

    int iBmp         = tbox->tools[(ptrdiff_t)i].img;
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

void Toolbox_OnPaint(Toolbox* tbox)
{
  PAINTSTRUCT ps;
  HDC hdc;
  hdc = BeginPaint(tbox->hwnd, &ps);

  Toolbox_DrawButtons(tbox, hdc);

  EndPaint(tbox->hwnd, &ps);
}

void Toolbox_OnLButtonUp(Toolbox* tbox, MOUSEEVENT mEvt)
{
  int x = LOWORD(mEvt.lParam);
  int y = HIWORD(mEvt.lParam);

  int btnHot = btnSize + 5;

  if (x >= 0 && y >= 0 && x < btnHot * 2 &&
      y < (int)tbox->tool_count * btnHot) {
    size_t extSize = btnSize + btnOffset;

    RECT rcClient = {0};
    GetClientRect(mEvt.hwnd, &rcClient);

    size_t rowCount = rcClient.right / extSize;
    if (rowCount < 1) {
      rowCount = 1;
    }

    unsigned int pressed =
        (y - btnOffset) / extSize * rowCount + (x - btnOffset) / extSize;

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
    }
  }

  InvalidateRect(tbox->hwnd, NULL, TRUE);
}

LRESULT CALLBACK Toolbox_WndProc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
  MOUSEEVENT mevt;
  mevt.hwnd   = hwnd;
  mevt.lParam = lParam;

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
    Toolbox* tbox = (Toolbox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (tbox) {
      Toolbox_OnLButtonUp(tbox, mevt);
    }
  } break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

void Toolbox_RegisterClass()
{
  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = Toolbox_WndProc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = TOOLBOX_WC;

  RegisterClass(&wc);
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

void ToolText_OnLButtonUp(MOUSEEVENT mEvt)
{
  HDC viewportdc = GetDC(g_viewport.hwnd);
  HDC bitmapdc   = CreateCompatibleDC(viewportdc);

  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  wchar_t textbuf[1024];
  DWORD textLen = 0;
  GetWindowText(g_option_bar.textstring_handle,
                textbuf,
                sizeof(textbuf) / sizeof(wchar_t));
  textLen = wcslen(textbuf);

  HFONT hFont = CreateFont(24, 0, 0, 0, 600, FALSE, FALSE, FALSE,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH, L"Arial");
  HGDIOBJ hOldFont = SelectObject(bitmapdc, hFont);

  SIZE size = {0};
  GetTextExtentPoint32(bitmapdc, textbuf, textLen, &size);
  RECT textrc = {0, 0, size.cx, size.cy};

  uint8_t* bmbuf = calloc(size.cx * size.cy, sizeof(uint32_t));

  HBITMAP hbitmap =
      CreateBitmap(size.cx, size.cy, 1, sizeof(uint32_t) * 8, bmbuf);
  SelectObject(bitmapdc, hbitmap);
  SetTextColor(bitmapdc, abgr_to_argb(g_color_context.fg_color));

  DrawTextEx(bitmapdc, textbuf, -1, &textrc, 0, NULL);

  SelectObject(bitmapdc, hOldFont);

  void* inbuf = malloc(size.cx * size.cy * 8);

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
  bmi.bmiHeader.biWidth = size.cx;
  bmi.bmiHeader.biHeight = size.cy;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  GetDIBits(bitmapdc, hbitmap, 0, size.cy, inbuf, &bmi, DIB_RGB_COLORS);
  Canvas_PasteBits(g_viewport.document->canvas, inbuf, x, y, size.cx, size.cy);

  free(inbuf);
  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);
}

void ToolText_Init()
{
  g_tool_text.label       = L"Text";
  g_tool_text.img         = 6;
  g_tool_text.onlbuttonup = ToolText_OnLButtonUp;
}

void ToolPencil_OnLButtonDown(MOUSEEVENT mEvt)
{
  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);
}

void ToolPencil_OnLButtonUp(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(mEvt.hwnd);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    Canvas* canvas = g_viewport.document->canvas;
    draw_line(canvas, prev.x, prev.y, x, y);

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());
}

void ToolPencil_OnMouseMove(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;
    hdc = GetDC(mEvt.hwnd);

    /* Draw overlay path */
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    /* Draw on canvas */
    Canvas* canvas = g_viewport.document->canvas;
    draw_line(canvas, prev.x, prev.y, x, y);

    prev.x = x;
    prev.y = y;

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
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

void ToolCircle_OnLButtonDown(MOUSEEVENT mEvt)
{
  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  circCenter.x = LOWORD(mEvt.lParam);
  circCenter.y = HIWORD(mEvt.lParam);
}

void ToolCircle_OnLButtonUp(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(mEvt.hwnd);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    int radius = sqrt(pow(x - circCenter.x, 2) + pow(y - circCenter.y, 2));

    Canvas* canvas = g_viewport.document->canvas;
    draw_circle(canvas, circCenter.x, circCenter.y, radius);

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
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

void ToolLine_OnLButtonDown(MOUSEEVENT mEvt)
{
  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);
}

void ToolLine_OnLButtonUp(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
    Canvas* canvas = g_viewport.document->canvas;
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

void ToolRectangle_OnLButtonDown(MOUSEEVENT mEvt)
{
  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);
}

void ToolRectangle_OnLButtonUp(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
    Canvas* canvas = g_viewport.document->canvas;
    draw_rectangle(canvas, prev.x, prev.y, x, y);
  }
  fDraw = FALSE;
  ReleaseCapture();
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
  if (q->capacity <= q->length)                                                \
  {                                                                            \
    q->data = realloc(q->data, (q->capacity + CAPACITY_GROW) * sizeof(T));     \
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

void ToolPicker_OnLButtonUp(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  uint32_t color = Canvas_GetPixel(g_viewport.document->canvas, x, y);
  SetForegroundColor(color);
}

void ToolPicker_OnRButtonUp(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  uint32_t color = Canvas_GetPixel(g_viewport.document->canvas, x, y);
  SetBackgroundColor(color);
}

void ToolFill_DoFloodFill(MOUSEEVENT mEvt, uint32_t newColor)
{
  History_StartDifferentiation(Panitent_GetActiveDocument());

  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  Canvas* canvas = g_viewport.document->canvas;
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

    Viewport_Invalidate(&g_viewport);

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

void ToolFill_OnRButtonUp(MOUSEEVENT mEvt)
{
  ToolFill_DoFloodFill(mEvt, g_color_context.bg_color);
}

void ToolFill_OnLButtonUp(MOUSEEVENT mEvt)
{
  ToolFill_DoFloodFill(mEvt, g_color_context.fg_color);
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

void ToolBrush_OnLButtonUp(MOUSEEVENT mEvt)
{
  fDraw = FALSE;
  ReleaseCapture();

  History_FinalizeDifferentiation(Panitent_GetActiveDocument());

  Brush_Delete(g_pBrushDraw);
}

void ToolBrush_OnLButtonDown(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  History_StartDifferentiation(Panitent_GetActiveDocument());

  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);


  g_pBrushDraw = BrushBuilder_Build(g_pBrush, g_brushSize);

  Brush_Draw(g_pBrushDraw, x, y, g_viewport.document->canvas, g_color_context.fg_color);
  Viewport_Invalidate();
}

void ToolBrush_OnMouseMove(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw)
  {
    Brush_DrawTo(g_pBrushDraw, prev.x, prev.y, x, y, g_viewport.document->canvas, g_color_context.fg_color);
    prev.x = x;
    prev.y = y;
    Viewport_Invalidate();
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

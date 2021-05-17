#include <windowsx.h>
#include "tool.h"
#include "viewport.h"
#include "canvas.h"
#include "document.h"
#include "color_context.h"
#include "commontypes.h"
#include "option_bar.h"
#include <assert.h>
#include "palette.h"
#include <math.h>

struct _Tool {
  LPWSTR lpszLabel;
  int img;
  void (*eventProc)(Viewport*, UINT, WPARAM, LPARAM);
};

Tool* g_tool;

void PointerTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void PencilTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void LineTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void CircleTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void RectangleTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void TextTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void FillTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void PickerTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void BreakerTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void PenTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);
void BrushTool_EvtProc(Viewport*, UINT, WPARAM, LPARAM);

void Tool_ProcessEvent(Tool* tool, Viewport* viewport, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  (*tool->eventProc)(viewport, message, wParam, lParam);
}

Tool* Tool_New(LPWSTR lpszLabel, int img, void (*eventProc)(Viewport*, UINT, WPARAM,
      LPARAM))
{
  Tool* tool = calloc(1, sizeof(Tool));
  tool->lpszLabel = lpszLabel;
  tool->img = img;
  tool->eventProc = eventProc;
  return tool;
}

void Tool_Free(Tool* tool)
{
  assert(tool);
  if (!tool)
    return;

  free(tool);
}

int Tool_GetImg(Tool* tool)
{
  return tool->img;
}

Tool* GetSelectedTool()
{
  return g_tool;
}

void SelectTool(Tool* tool)
{
  g_tool = tool;
}

Tool* g_pointer;
Tool* g_pencilTool;
Tool* g_circTool;
Tool* g_lineTool;
Tool* g_rectTool;
Tool* g_textTool;
Tool* g_fillTool;
Tool* g_pickTool;
Tool* g_breakTool;
Tool* g_penTool;
Tool* g_brushTool;

Tool* GetDefaultTool(DefaultTool type)
{
  switch (type)
  {
    case E_TOOL_POINTER:
      return g_pointer;
    case E_TOOL_PENCIL:
      return g_pencilTool;
    case E_TOOL_LINE:
      return g_lineTool;
    case E_TOOL_CIRCLE:
      return g_circTool;
    case E_TOOL_RECTANGLE:
      return g_rectTool;
    case E_TOOL_TEXT:
      return g_textTool;
    case E_TOOL_FILL:
      return g_fillTool;
    case E_TOOL_PICKER:
      return g_pickTool;
    case E_TOOL_BREAKER:
      return g_breakTool;
    case E_TOOL_PEN:
      return g_penTool;
    case E_TOOL_BRUSH:
      return g_brushTool;
  }

  return g_pointer;
}

void InitDefaultTools()
{
  g_pointer = Tool_New(L"Pointer", 0, PointerTool_EvtProc);
  g_pencilTool = Tool_New(L"Pencil", 1, PencilTool_EvtProc);
  g_circTool = Tool_New(L"Circle", 2, CircleTool_EvtProc);
  g_lineTool = Tool_New(L"Line", 3, LineTool_EvtProc);
  g_rectTool = Tool_New(L"Rectangle", 4, RectangleTool_EvtProc);
  g_textTool = Tool_New(L"Text", 6, TextTool_EvtProc);
  g_fillTool = Tool_New(L"Flood fill", 8, FillTool_EvtProc);
  g_pickTool = Tool_New(L"Color picker", 9, PickerTool_EvtProc);
  g_breakTool = Tool_New(L"Application breaker", 10, BreakerTool_EvtProc);
  g_penTool = Tool_New(L"Pen", 11, PenTool_EvtProc);
  g_brushTool = Tool_New(L"Brush", 12, BrushTool_EvtProc);
}

void PointerTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  /* Do nothing */
}

void PointerTool_Init()
{
  Tool* pointer = Tool_New(L"Pointer", 0, PointerTool_EvtProc);
}

void TextTool_OnLButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  Document* document = Viewport_GetDocument(viewport);
  Canvas* canvas = Document_GetCanvas(document);

  HDC viewportdc = GetDC(Viewport_GetHWND(viewport));
  HDC bitmapdc   = CreateCompatibleDC(viewportdc);

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
  Canvas_PasteBits(canvas, inbuf, x, y, size.cx, size.cy);

  free(inbuf);
  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);
}

void TextTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_LBUTTONUP:
      TextTool_OnLButtonUp(viewport, wParam, lParam);
      break;
  }
}

static Rect prevRc;
static BOOL fDraw = FALSE;

void PencilTool_OnLButtonDown(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  fDraw = TRUE;
  SetCapture(Viewport_GetHWND(viewport));
  prevRc.left = x;
  prevRc.top  = y;
}

void PencilTool_OnLButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(mEvt.hwnd);
    MoveToEx(hdc, prevRc.left, prevRc.top, NULL);
    LineTo(hdc, x, y);
#endif

    Document* document = Viewport_GetDocument(viewport);
    Canvas* canvas = Document_GetCanvas(document);

    Rect rc = {0};
    rc.left   = prevRc.left;
    rc.top    = prevRc.top;
    rc.right  = x;
    rc.bottom = y;

    Canvas_DrawLine(canvas, rc);

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void PencilTool_OnMouseMove(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;
    hdc = GetDC(Viewport_GetHWND());

    /* Draw overlay path */
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    /* Draw on canvas */

    Document* document = Viewport_GetDocument(viewport);
    Canvas* canvas = Document_GetCanvas(document);

    Rect rc = {0};
    rc.left   = prevRc.left;
    rc.top    = prevRc.top;
    rc.right  = x;
    rc.bottom = y;

    Canvas_DrawLine(canvas, rc);

    prevRc.left = x;
    prevRc.top  = y;

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
}

void PencilTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_LBUTTONDOWN:
      PencilTool_OnLButtonDown(viewport, wParam, lParam);
      break;
    case WM_LBUTTONUP:
      PencilTool_OnLButtonUp(viewport, wParam, lParam);
      break;
    case WM_MOUSEMOVE:
      PencilTool_OnMouseMove(viewport, wParam, lParam);
      break;
  }
}

POINT circCenter;

void CircleTool_OnLButtonDown(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  fDraw = TRUE;
  SetCapture(Viewport_GetHWND(viewport));
  circCenter.x = x;
  circCenter.y = y;
}

void CircleTool_OnLButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  if (fDraw) {
    x = GET_X_LPARAM(lParam);
    y = GET_Y_LPARAM(lParam);

#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(mEvt.hwnd);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    int radius = sqrt(pow(x - circCenter.x, 2) + pow(y - circCenter.y, 2));

    Document* document = Viewport_GetDocument(viewport);
    Canvas* canvas = Document_GetCanvas(document);

    Canvas_DrawCircle(canvas, circCenter.x, circCenter.y, radius);

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void CircleTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_LBUTTONDOWN:
      CircleTool_OnLButtonDown(viewport, wParam, lParam);
      break;
    case WM_LBUTTONUP:
      CircleTool_OnLButtonUp(viewport, wParam, lParam);
      break;
  }
}

void LineTool_OnLButtonDown(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  fDraw = TRUE;
  SetCapture(Viewport_GetHWND(viewport));
  prevRc.right = GET_X_LPARAM(lParam);
  prevRc.bottom = GET_Y_LPARAM(lParam);
}

void LineTool_OnLButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  signed short x;
  signed short y;

  if (fDraw) {
    x = GET_X_LPARAM(lParam);
    y = GET_Y_LPARAM(lParam);

    Document* document = Viewport_GetDocument(viewport);
    Canvas* canvas = Document_GetCanvas(document); 

    Rect rc = {0};
    rc.left   = prevRc.left;
    rc.top    = prevRc.top;
    rc.right  = x;
    rc.bottom = y;

    Canvas_DrawLine(canvas, rc);
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void LineTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_LBUTTONDOWN:
      LineTool_OnLButtonDown(viewport, wParam, lParam);
      break;
    case WM_LBUTTONUP:
      LineTool_OnLButtonUp(viewport, wParam, lParam);
      break;
  }
}

void LineTool_Init()
{
}

void RectangleTool_OnLButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  fDraw = TRUE;
  SetCapture(Viewport_GetHWND(viewport));
  prevRc.left = GET_X_LPARAM(lParam);
  prevRc.top = GET_Y_LPARAM(lParam);
}

void RectangleTool_OnLButtonDown(Viewport* viewport, WPARAM wParam,
    LPARAM lParam)
{
  signed short x = GET_X_LPARAM(lParam);
  signed short y = GET_Y_LPARAM(lParam); 

  if (fDraw)
  {
    Document* document = Viewport_GetDocument(viewport);
    Canvas* canvas = Document_GetCanvas(document);

    Rect rc = {0};
    rc.left   = prevRc.left;
    rc.top    = prevRc.top;
    rc.right  = x;
    rc.bottom = y;

    Canvas_DrawRectangle(canvas, rc);
  }
 
  fDraw = FALSE; 
  ReleaseCapture();
}

void RectangleTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_LBUTTONDOWN:
      RectangleTool_OnLButtonDown(viewport, wParam, lParam);
      break;
    case WM_LBUTTONUP:
      RectangleTool_OnLButtonUp(viewport, wParam, lParam);
      break;
  }
}

void FillTool_Actuate(Viewport* viewport, WPARAM wParam, LPARAM lParam,
    uint32_t color)
{
  Document* document;
  Canvas* canvas;
  signed short x;
  signed short y;

  document = Viewport_GetDocument(viewport);
  canvas = Document_GetCanvas(document);

  x = GET_X_LPARAM(lParam);
  y = GET_Y_LPARAM(lParam);

  Canvas_FloodFill(canvas, x, y, GetForegroundColor());
}

void FillTool_OnLButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  FillTool_Actuate(viewport, wParam, lParam, GetForegroundColor());
}

void FillTool_OnRButtonUp(Viewport* viewport, WPARAM wParam, LPARAM lParam)
{
  FillTool_Actuate(viewport, wParam, lParam, GetBackgroundColor());
}

void FillTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_RBUTTONUP:
      FillTool_OnRButtonUp(viewport, wParam, lParam);
      break;
    case WM_LBUTTONUP:
      FillTool_OnLButtonUp(viewport, wParam, lParam);
      break;
  }
}

void PickerTool_OnLButtonUp(Viewport* viewport, WPARAM wParam,
    LPARAM lParam)
{
  assert(viewport);

  Document* document = Viewport_GetDocument(viewport);
  Canvas* canvas = Document_GetCanvas(document);
  assert(canvas);

  uint32_t color = Canvas_GetPixel(canvas, LOWORD(lParam), HIWORD(lParam));
  SetBackgroundColor(color);
}

void PickerTool_OnRButtonUp(Viewport* viewport, WPARAM wParam,
    LPARAM lParam)
{
  assert(viewport);

  Document* document = Viewport_GetDocument(viewport);
  Canvas* canvas = Document_GetCanvas(document);
  assert(canvas);

  uint32_t color = Canvas_GetPixel(canvas, LOWORD(lParam), HIWORD(lParam));
  SetForegroundColor(color);
}

void PickerTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_LBUTTONUP:
      PickerTool_OnLButtonUp(viewport, wParam, lParam);
      break;
    case WM_RBUTTONUP:
      PickerTool_OnRButtonUp(viewport, wParam, lParam);
      break;
  }
}

void BreakerTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  *((int *)0xFFFFFFFF) = 1;
}

void PenTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void BrushTool_EvtProc(Viewport* viewport, UINT message, WPARAM wParam, LPARAM lParam)
{

}

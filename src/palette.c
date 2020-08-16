#include "palette.h"
#include <stdint.h>
#include <windowsx.h>

struct color_context {
  
};

uint32_t selected_color;

uint32_t abgr_to_argb(uint32_t abgr)
{
  uint32_t b = (abgr>>16) & 0xFF;
  uint32_t r = abgr & 0xFF;
  return (abgr & 0xFF00FF00) | (r << 16) | b;
}

void draw_swatch(HDC hdc, int x, int y, COLORREF color)
{
  HDC original = SelectObject(hdc, GetStockObject(DC_BRUSH));

  SetDCBrushColor(hdc, color);
  Rectangle(hdc, x, y, x+24, y+24);

  SelectObject(hdc, original);
}

void palette_window_onpaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);

  uint32_t color = 0x00ff0000;
  draw_swatch(hdc, 10, 10, (COLORREF)selected_color);

  for (size_t i = 0; color; i++)
  {
    draw_swatch(hdc, 10 + (i % 8) * 26, 40 + (i / 8) * 26,
        (COLORREF)(color));

    color >>= 1;
        
  }

  EndPaint(hwnd, &ps);
}

int transform_pos_to_index(int x, int y)
{
  x -= 10;
  y -= 40;

  int index = y / 26 * 8 + x / 26;
  printf("Index: %d\n", index);

  return index;
}

uint32_t get_color(int index)
{
  return 0x00ff0000 >> index;
}

void palette_window_onbuttonup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < (26 * 8)) && ((y - 40) < (26 * 3)))
  {
    int index = transform_pos_to_index(x, y); 
    selected_color = get_color(index);

    HDC hdc = GetDC(hwnd);
    RECT rc = {10, 10, 34, 34};
    InvalidateRect(hwnd, &rc, FALSE);
  }
}

LRESULT CALLBACK palette_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
    case WM_CREATE:
        
      break;
    case WM_PAINT:
      palette_window_onpaint(hwnd, wparam, lparam);
      break;
    case WM_LBUTTONUP:
      palette_window_onbuttonup(hwnd, wparam, lparam);
      break;
    default:
      return DefWindowProc(hwnd, message, wparam, lparam);
      break;
  }
  
  return 0;
}

void register_palette_dialog(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)palette_window_proc;
  wcex.cbWndExtra = 0;
  wcex.cbClsExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_PaletteWindow";
  wcex.lpszMenuName = NULL;
  wcex.hIconSm = NULL;
  ATOM classAtom = RegisterClassEx(&wcex);
  if (!classAtom)
    printf("Failed to register PaletteWindow");
}

void palette_dialog_onpaint(HWND hwnd)
{
  PAINTSTRUCT ps = {};
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);
  Rectangle(hdc, 2, 2, 24, 24);
}

void init_palette_window(HWND parent)
{
  HWND hPalette = CreateWindowEx(WS_EX_TOOLWINDOW,
      L"Win32Class_PaletteWindow", L"Palette",
      WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL,
      GetModuleHandle(NULL), NULL);

  if (!hPalette)
    printf("Failed to create window");
}

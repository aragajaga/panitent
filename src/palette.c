#include "precomp.h"

#include <windowsx.h>
#include <stdint.h>

#include "color_context.h"
#include "palette.h"

palette_dialog_t g_palette_dialog;

uint32_t palette_colors[] = {
  0x00000000, // Black
  0x00FFFFFF, // White
  0x00CCCCCC, // Light Gray
  0x00888888, // Gray
  0x00444444, // Dark Gray

  0x00FF8888, // Light Red
  0x00FFFF88, // Light Yellow
  0x0088FF88, // Light Green
  0x0088FFFF, // Light Cyan
  0x008888FF, // Light Blue 
  0x00FF88FF, // Light Magenta

  0x00FF0000, // Red
  0x00FFFF00, // Yellow
  0x0000FF00, // Green
  0x0000FFFF, // Cyan
  0x000000FF, // Blue 
  0x00FF00FF, // Magenta

  0x00880000, // Crimson
  0x00888800, // Gold 
  0x00008800, // Dark Green 
  0x00008888, // Dark Cyan 
  0x00000088, // Dark Blue 
  0x00880088, // Dark Magenta 
};

uint32_t abgr_to_argb(uint32_t abgr)
{
  uint32_t b = (abgr>>16) & 0xFF;
  uint32_t r = abgr & 0xFF;
  return (abgr & 0xFF00FF00) | (r << 16) | b;
}

int swatch_size = 16;
int swatch_margin = 2;

void draw_swatch(HDC hdc, int x, int y, COLORREF color)
{
  HDC original = SelectObject(hdc, GetStockObject(DC_BRUSH));

  SetDCBrushColor(hdc, color);
  Rectangle(hdc, x, y, x+swatch_size, y+swatch_size);

  SelectObject(hdc, original);
}

void palette_window_onpaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;

  RECT rc = {};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);

  if (width_indices < 1)
    width_indices = 1;


  hdc = BeginPaint(hwnd, &ps);

  draw_swatch(hdc, 10, 10,
      (COLORREF)abgr_to_argb(g_color_context.fg_color));

  draw_swatch(hdc, 40, 10,
      (COLORREF)abgr_to_argb(g_color_context.bg_color));

  uint32_t color = 0x00ff0000;
  for (size_t i = 0; i < sizeof(palette_colors) / sizeof(uint32_t); i++)
  {
    draw_swatch(hdc, 10 + (i % width_indices) * (swatch_size + swatch_margin), 40 + (i / width_indices) * (swatch_size + swatch_margin),
        (COLORREF)(palette_colors[i]));

    color >>= 1;
  }

  EndPaint(hwnd, &ps);
}

int transform_pos_to_index(int x, int y)
{
  RECT rc = {};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int swatch_outer = swatch_size + swatch_margin;

  x -= 10;
  y -= 40;

  int index = y / swatch_outer * width_indices + x / swatch_outer;
  printf("Index: %d\n", index);

  return index;
}

uint32_t get_color(int index)
{
  return palette_colors[index];
}

void palette_window_onbuttonup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  RECT rc = {};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int swatch_count = sizeof(palette_colors) / sizeof(uint32_t);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) && ((y - 40) < ((swatch_size + swatch_margin) * height_indices)))
  {
    int index = transform_pos_to_index(x, y); 
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.fg_color = abgr_to_argb(get_color(index));

    HDC hdc = GetDC(hwnd);
    RECT rc = {10, 10, 34, 34};
    InvalidateRect(hwnd, &rc, FALSE);
  }
}

void palette_window_onrbuttonup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  RECT rc = {};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int swatch_count = sizeof(palette_colors) / sizeof(uint32_t);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) && ((y - 40) < ((swatch_size + swatch_margin) * height_indices)))
  {
    int index = transform_pos_to_index(x, y); 
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.bg_color = abgr_to_argb(get_color(index));

    HDC hdc = GetDC(hwnd);
    RECT rc = {40, 10, 64, 34};
    InvalidateRect(hwnd, &rc, FALSE);
  }
}

LRESULT CALLBACK palette_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
    case WM_CREATE:
      g_palette_dialog.win_handle = hwnd;  
      break;
    case WM_PAINT:
      palette_window_onpaint(hwnd, wparam, lparam);
      break;
    case WM_LBUTTONUP:
      palette_window_onbuttonup(hwnd, wparam, lparam);
      break;
    case WM_RBUTTONUP:
      palette_window_onrbuttonup(hwnd, wparam, lparam);
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
  HWND hPalette = CreateWindowEx(WS_EX_PALETTEWINDOW,
      L"Win32Class_PaletteWindow", L"Palette",
      WS_CAPTION | WS_THICKFRAME | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL,
      GetModuleHandle(NULL), NULL);

  if (!hPalette)
    printf("Failed to create window");
}

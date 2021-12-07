#include "precomp.h"

#include <stdint.h>
#include <stdio.h>
#include <windowsx.h>
#include <assert.h>

#include "palette.h"
#include "resource.h"
#include "swatch2.h"
#include "checker.h"

#include "color_context.h"

#define IDM_PALETTESETTINGS 101

palette_dialog_t g_palette_dialog;

uint32_t palette_colors[] = {
    0xFF000000, /* Black */
    0xFFFFFFFF, /* White */
    0xFFCCCCCC, /* Light Gray */
    0xFF888888, /* Gray */
    0xFF444444, /* Dark Gray */

    0xFFFF8888, /* Light Red */
    0xFFFFFF88, /* Light Yellow */
    0xFF88FF88, /* Light Green */
    0xFF88FFFF, /* Light Cyan */
    0xFF8888FF, /* Light Blue */
    0xFFFF88FF, /* Light Magenta */

    0xFFFF0000, /* Red */
    0xFFFFFF00, /* Yellow */
    0xFF00FF00, /* Green */
    0xFF00FFFF, /* Cyan */
    0xFF0000FF, /* Blue */
    0xFFFF00FF, /* Magenta */

    0xFF880000, /* Crimson */
    0xFF888800, /* Gold */
    0xFF008800, /* Dark Green */
    0xFF008888, /* Dark Cyan */
    0xFF000088, /* Dark Blue */
    0xFF880088, /* Dark Magenta */

    0xCC000000, /* Black */
    0xCCFFFFFF, /* White */
    0xCCCCCCCC, /* Light Gray */
    0xCC888888, /* Gray */
    0xCC444444, /* Dark Gray */

    0xCCFF8888, /* Light Red */
    0xCCFFFF88, /* Light Yellow */
    0xCC88FF88, /* Light Green */
    0xCC88FFFF, /* Light Cyan */
    0xCC8888FF, /* Light Blue */
    0xCCFF88FF, /* Light Magenta */

    0xCCFF0000, /* Red */
    0xCCFFFF00, /* Yellow */
    0xCC00FF00, /* Green */
    0xCC00FFFF, /* Cyan */
    0xCC0000FF, /* Blue */
    0xCCFF00FF, /* Magenta */

    0xCC880000, /* Crimson */
    0xCC888800, /* Gold */
    0xCC008800, /* Dark Green */
    0xCC008888, /* Dark Cyan */
    0xCC000088, /* Dark Blue */
    0xCC880088, /* Dark Magenta */

    0x88000000, /* Black */
    0x88FFFFFF, /* White */
    0x88CCCCCC, /* Light Gray */
    0x88888888, /* Gray */
    0x88444444, /* Dark Gray */

    0x88FF8888, /* Light Red */
    0x88FFFF88, /* Light Yellow */
    0x8888FF88, /* Light Green */
    0x8888FFFF, /* Light Cyan */
    0x888888FF, /* Light Blue */
    0x88FF88FF, /* Light Magenta */

    0x88FF0000, /* Red */
    0x88FFFF00, /* Yellow */
    0x8800FF00, /* Green */
    0x8800FFFF, /* Cyan */
    0x880000FF, /* Blue */
    0x88FF00FF, /* Magenta */

    0x88880000, /* Crimson */
    0x88888800, /* Gold */
    0x88008800, /* Dark Green */
    0x88008888, /* Dark Cyan */
    0x88000088, /* Dark Blue */
    0x88880088, /* Dark Magenta */
};

typedef struct _PaletteWindowData {
  HBRUSH hbrChecker;
} PaletteWindowData;

INT_PTR CALLBACK PaletteSettingsDlgProc(HWND hwndDlg, UINT message,
    WPARAM wParam, LPARAM lParam);

uint32_t abgr_to_argb(uint32_t abgr)
{
  uint32_t b = (abgr >> 16) & 0xFF;
  uint32_t r = abgr & 0xFF;
  return (abgr & 0xFF00FF00) | (r << 16) | b;
}

struct _PaletteSettings {
  int swatchSize;
  int checkerSize;
  uint32_t checkerColor1;
  uint32_t checkerColor2;
  HBRUSH checkerBrush;
  BOOL checkerInvalidate;
};

struct _PaletteSettings g_paletteSettings = {
  .swatchSize = 16,
  .checkerSize = 8,
  .checkerColor1 = 0xFFCCCCCC,
  .checkerColor2 = 0xFFFFFFFF,
  .checkerInvalidate = TRUE
};

int swatch_margin = 2;

void PaletteWindow_DrawSwatch(HWND hwnd, HDC hdc, int x, int y, uint32_t color)
{
  PaletteWindowData *data = (PaletteWindowData*)
      GetWindowLongPtr(hwnd, GWLP_USERDATA);

  int swatchSize = g_paletteSettings.swatchSize;

  SetBrushOrgEx(hdc, x, y, NULL);

  RECT checkerRc = {
    x, y, x + swatchSize, y + swatchSize
  };
  FillRect(hdc, &checkerRc, data->hbrChecker);

  HDC hdcFill = CreateCompatibleDC(hdc);
  HBITMAP hbmFill = CreateCompatibleBitmap(hdc, swatchSize, swatchSize);
  HGDIOBJ hOldObj = SelectObject(hdcFill, hbmFill);

  HBRUSH hbrColor = CreateSolidBrush(color & 0x00FFFFFF);
  RECT swatchRc = {0, 0, swatchSize, swatchSize};
  FillRect(hdcFill, &swatchRc, hbrColor);
  DeleteObject(hbrColor);

  BLENDFUNCTION blendFunc = {
    .BlendOp = AC_SRC_OVER,
    .BlendFlags = 0,
    .SourceConstantAlpha = color >> 24,
    .AlphaFormat = 0
  };

  AlphaBlend(hdc, x, y, swatchSize, swatchSize, hdcFill, 0, 0, swatchSize,
      swatchSize, blendFunc);

  SelectObject(hdcFill, hOldObj);

  DeleteObject(hbmFill);
  DeleteDC(hdcFill);

  RECT frameRc = {x, y, x+swatchSize, y+swatchSize};
  FrameRect(hdc, &frameRc, GetStockObject(BLACK_BRUSH));
}

void PaletteWindow_OnPaint(HWND hwnd)
{
  int swatch_size = g_paletteSettings.swatchSize;

  PAINTSTRUCT ps;
  HDC hdc;

  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);

  if (width_indices < 1)
    width_indices = 1;

  hdc = BeginPaint(hwnd, &ps);

  PaletteWindow_DrawSwatch(hwnd, hdc, 10, 10,
      (COLORREF)abgr_to_argb(g_color_context.fg_color));
  PaletteWindow_DrawSwatch(hwnd, hdc, 40, 10,
      (COLORREF)abgr_to_argb(g_color_context.bg_color));

  uint32_t color = 0x00ff0000;
  for (size_t i = 0; i < sizeof(palette_colors) / sizeof(uint32_t); i++) {
    PaletteWindow_DrawSwatch(hwnd, hdc,
                10 + ((int)i % width_indices) * (swatch_size + swatch_margin),
                40 + ((int)i / width_indices) * (swatch_size + swatch_margin),
                palette_colors[i]);

    color >>= 1;
  }

  EndPaint(hwnd, &ps);
}

int PaletteWindow_PosToSwatchIndex(int x, int y)
{
  int swatch_size = g_paletteSettings.swatchSize;

  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int swatch_outer  = swatch_size + swatch_margin;

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

void PaletteWindow_OnLButtonUp(HWND hwnd, LPARAM lParam)
{
  int swatch_size = g_paletteSettings.swatchSize;

  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int swatch_count   = sizeof(palette_colors) / sizeof(uint32_t);
  int width_indices  = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) &&
      ((y - 40) < ((swatch_size + swatch_margin) * height_indices))) {
    int index = PaletteWindow_PosToSwatchIndex(x, y);
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.fg_color = abgr_to_argb(get_color(index));

    RECT rcInvalidate = {10, 10, 34, 34};
    InvalidateRect(hwnd, &rcInvalidate, FALSE);
  }
}

void PaletteWindow_OnRButtonUp(HWND hwnd, LPARAM lParam)
{
  int swatch_size = g_paletteSettings.swatchSize;

  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int swatch_count   = sizeof(palette_colors) / sizeof(uint32_t);
  int width_indices  = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) &&
      ((y - 40) < ((swatch_size + swatch_margin) * height_indices))) {
    int index = PaletteWindow_PosToSwatchIndex(x, y);
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.bg_color = abgr_to_argb(get_color(index));

    RECT rcInvalidate = {40, 10, 64, 34};
    InvalidateRect(hwnd, &rcInvalidate, FALSE);
  }
}

void Palette_ColorChangeObserver(void* userData, uint32_t fg, uint32_t bg)
{
  UNREFERENCED_PARAMETER(fg);
  UNREFERENCED_PARAMETER(bg);

  HWND hWnd = (HWND)userData;
  if (!hWnd)
    return;

  InvalidateRect(hWnd, NULL, TRUE);
}

static void OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);

  if (LOWORD(wParam) == IDM_PALETTESETTINGS)
  {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PALETTESETTINGS),
        hwnd, (DLGPROC)PaletteSettingsDlgProc);
  }
}

static void OnContextMenu(HWND hwnd, int x, int y)
{
  x = x < 0 ? 0 : x;
  y = y < 0 ? 0 : y;

  HMENU hMenu = CreatePopupMenu();
  InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_PALETTESETTINGS,
      L"Settings");
  TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, hwnd, NULL);
}

void PaletteWindow_OnCreate(HWND hwnd)
{
  PaletteWindowData *data = (PaletteWindowData*)
    malloc(sizeof(PaletteWindowData));
  if (!data)
    return;

  ZeroMemory(data, sizeof(PaletteWindowData));
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);

  CheckerConfig checkerCfg;
  checkerCfg.tileSize = 8;
  checkerCfg.color[0] = 0x00CCCCCC;
  checkerCfg.color[1] = 0x00FFFFFF;

  HDC hdc = GetDC(hwnd);
  data->hbrChecker = CreateChecker(&checkerCfg, hdc);
  ReleaseDC(hwnd, hdc);

  RegisterColorObserver(Palette_ColorChangeObserver, (void*)hwnd);
  g_palette_dialog.win_handle = hwnd;
}

LRESULT CALLBACK PaletteWindow_WndProc(HWND hwnd,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam)
{
  switch (message) {
  case WM_CREATE:
    PaletteWindow_OnCreate(hwnd);
    break;
  case WM_PAINT:
    PaletteWindow_OnPaint(hwnd);
    break;
  case WM_LBUTTONUP:
    PaletteWindow_OnLButtonUp(hwnd, lParam);
    return TRUE;
  case WM_RBUTTONUP:
    PaletteWindow_OnRButtonUp(hwnd, lParam);
    DefWindowProc(hwnd, message, wParam, lParam);
    return TRUE;
  case WM_CONTEXTMENU:
    OnContextMenu(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    break;
  case WM_COMMAND:
    OnCommand(hwnd, wParam, lParam);
    break;
  case WM_DESTROY:
    RemoveColorObserver(Palette_ColorChangeObserver, (void*)hwnd);
    break;
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
    break;
  }

  return 0;
}

BOOL PaletteWindow_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  ZeroMemory(&wcex, sizeof(wcex));

  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = (WNDPROC)PaletteWindow_WndProc;
  wcex.hInstance     = hInstance;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_PaletteWindow";

  return RegisterClassEx(&wcex);
}

void PaletteWindow_Create(HWND parent)
{
  HWND hPalette = CreateWindowEx(
      WS_EX_PALETTEWINDOW, L"Win32Class_PaletteWindow", L"Palette",
      WS_CAPTION | WS_THICKFRAME | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
      parent, NULL, GetModuleHandle(NULL), NULL);

  if (!hPalette)
    printf("Failed to create window");
}

#define SWATCH_SIZE_LEN 4
#define CHECKER_SIZE_LEN 4

INT_PTR CALLBACK PaletteSettingsDlgProc(HWND hwndDlg, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);

  switch (message)
  {
    case WM_INITDIALOG:
      {
        WCHAR szSwatchSize[SWATCH_SIZE_LEN];
        _itow_s(g_paletteSettings.swatchSize, szSwatchSize, SWATCH_SIZE_LEN, 10);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_SWATCHSIZEEDIT), szSwatchSize);

        WCHAR szCheckerSize[CHECKER_SIZE_LEN];
        _itow_s(g_paletteSettings.checkerSize, szCheckerSize, CHECKER_SIZE_LEN, 10);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_CHECKERSIZEEDIT), szCheckerSize);

        SwatchControl2_SetColor(GetDlgItem(hwndDlg, IDC_CHECKERCOLOR1),
            g_paletteSettings.checkerColor1 & 0x00FFFFFF);
        SwatchControl2_SetColor(GetDlgItem(hwndDlg, IDC_CHECKERCOLOR2),
            g_paletteSettings.checkerColor2 & 0x00FFFFFF);
        return TRUE;
      }
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
        switch(LOWORD(wParam))
        {

          case IDOK:
          {
            WCHAR szSwatchSize[4];
            Edit_GetText(GetDlgItem(hwndDlg, IDC_SWATCHSIZEEDIT), szSwatchSize,
                4);
            g_paletteSettings.swatchSize = _wtoi(szSwatchSize);

            WCHAR szCheckerSize[4];
            Edit_GetText(GetDlgItem(hwndDlg, IDC_CHECKERSIZEEDIT),
                szCheckerSize, 4);
            g_paletteSettings.checkerSize = _wtoi(szCheckerSize);

            g_paletteSettings.checkerColor1 = SwatchControl2_GetColor(
                GetDlgItem(hwndDlg, IDC_CHECKERCOLOR1));

            g_paletteSettings.checkerColor2 = SwatchControl2_GetColor(
                GetDlgItem(hwndDlg, IDC_CHECKERCOLOR2));

            g_paletteSettings.checkerInvalidate = TRUE;

            InvalidateRect(GetParent(hwndDlg), NULL, TRUE);
            EndDialog(hwndDlg, wParam);
          }
            break;
          case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            break;
        }
      }
      return TRUE;
  }

  return FALSE;
}

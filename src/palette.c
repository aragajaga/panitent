#include "precomp.h"

#include <stdint.h>
#include <stdio.h>
#include <windowsx.h>
#include <assert.h>

#include "palette.h"
#include "resource.h"
#include "swatch2.h"

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

HBRUSH PaletteWindow_GetCheckerBrush(HDC hdc)
{
  if (g_paletteSettings.checkerInvalidate)
  {
    if (g_paletteSettings.checkerBrush)
    {
      DeleteObject(g_paletteSettings.checkerBrush);
    }

    int checkerSize = g_paletteSettings.checkerSize;
    uint32_t checkerColor1 = g_paletteSettings.checkerColor1;
    uint32_t checkerColor2 = g_paletteSettings.checkerColor2;

    HBITMAP hPatBmp = CreateCompatibleBitmap(hdc, checkerSize,
        checkerSize);

    HDC hPatDC = CreateCompatibleDC(hdc);
    SelectObject(hPatDC, hPatBmp);

    HBRUSH hbr1 = CreateSolidBrush(checkerColor1 & 0x00FFFFFF);
    HBRUSH hbr2 = CreateSolidBrush(checkerColor2 & 0x00FFFFFF);

    RECT rcBack = {0, 0, checkerSize, checkerSize};
    RECT rcLump1 = {0, 0, checkerSize/2, checkerSize/2};
    RECT rcLump2 = {checkerSize/2, checkerSize/2, checkerSize,
        checkerSize};

    FillRect(hPatDC, &rcBack, hbr2);
    FillRect(hPatDC, &rcLump1, hbr1);
    FillRect(hPatDC, &rcLump2, hbr1);

    DeleteObject(hbr1);
    DeleteObject(hbr2);

    /*
    HGDIOBJ hOldObj = SelectObject(hPatDC, GetStockObject(DC_BRUSH));

    SetDCBrushColor(hPatDC, checkerColor2 & 0x00FFFFFF);
    Rectangle(hPatDC, 0, 0, checkerSize, checkerSize);

    SetDCBrushColor(hPatDC, checkerColor1 & 0x00FFFFFF);
    Rectangle(hPatDC, 0, 0, checkerSize/2, checkerSize/2);
    Rectangle(hPatDC, checkerSize/2, checkerSize/2, checkerSize, checkerSize);

    SelectObject(hPatDC, hOldObj);
    */

    HBRUSH hCheckerBrush = CreatePatternBrush(hPatBmp);
    assert(hCheckerBrush);
    g_paletteSettings.checkerBrush = hCheckerBrush;

    g_paletteSettings.checkerInvalidate = FALSE;
  }

  return g_paletteSettings.checkerBrush;
}

static void PaletteWindow_DrawCheckerTexture(HDC hdc, int x, int y, int width,
    int height)
{
  RECT rc = {x, y, x+width, y+height};
  HBRUSH hbrChecker = PaletteWindow_GetCheckerBrush(hdc);
  FillRect(hdc, &rc, hbrChecker);

}

void PaletteWindow_DrawSwatch(HDC hdc, int x, int y, uint32_t color)
{
  int swatchSize = g_paletteSettings.swatchSize;

  /*
  HDC original = SelectObject(hdc, GetStockObject(DC_BRUSH));

  SetDCBrushColor(hdc, color);
  Rectangle(hdc, x, y, x + swatch_size, y + swatch_size);

  SelectObject(hdc, original);
  */

  SetBrushOrgEx(hdc, x, y, NULL);
  PaletteWindow_DrawCheckerTexture(hdc, x, y, swatchSize, swatchSize);

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

  PaletteWindow_DrawSwatch(hdc, 10, 10,
      (COLORREF)abgr_to_argb(g_color_context.fg_color));
  PaletteWindow_DrawSwatch(hdc, 40, 10,
      (COLORREF)abgr_to_argb(g_color_context.bg_color));

  uint32_t color = 0x00ff0000;
  for (size_t i = 0; i < sizeof(palette_colors) / sizeof(uint32_t); i++) {
    PaletteWindow_DrawSwatch(hdc,
                10 + (i % width_indices) * (swatch_size + swatch_margin),
                40 + (i / width_indices) * (swatch_size + swatch_margin),
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

    RECT rc = {10, 10, 34, 34};
    InvalidateRect(hwnd, &rc, FALSE);
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

    RECT rc = {40, 10, 64, 34};
    InvalidateRect(hwnd, &rc, FALSE);
  }
}

void Palette_ColorChangeObserver(void* userData, uint32_t fg, uint32_t bg)
{
  HWND hWnd = (HWND)userData;
  if (!hWnd)
    return;

  InvalidateRect(hWnd, NULL, TRUE);
}

static void OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
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

LRESULT CALLBACK PaletteWindow_WndProc(HWND hwnd,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam)
{
  switch (message) {
  case WM_CREATE:
    {
      RegisterColorObserver(Palette_ColorChangeObserver, (void*)hwnd);
      g_palette_dialog.win_handle = hwnd;
    }
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

void PaletteWindow_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex    = {0};
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = (WNDPROC)PaletteWindow_WndProc;
  wcex.cbWndExtra    = 0;
  wcex.cbClsExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = NULL;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_PaletteWindow";
  wcex.lpszMenuName  = NULL;
  wcex.hIconSm       = NULL;
  ATOM classAtom     = RegisterClassEx(&wcex);

  if (!classAtom)
    printf("Failed to register PaletteWindow");
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

INT_PTR CALLBACK PaletteSettingsDlgProc(HWND hwndDlg, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      {
        WCHAR szSwatchSize[4];
        _itow(g_paletteSettings.swatchSize, szSwatchSize, 10);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_SWATCHSIZEEDIT), szSwatchSize);

        WCHAR szCheckerSize[4];
        _itow(g_paletteSettings.checkerSize, szCheckerSize, 10);
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

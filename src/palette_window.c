#include "precomp.h"

#include "win32/window.h"
#include "palette_window.h"
#include "palette_settings_dlg.h"
#include "checker.h"
#include "color_context.h"
#include "resource.h"
#include "util.h"
#include "palette.h"
#include "panitent.h"

static const WCHAR szClassName[] = L"PaletteWindowClass";

static int swatch_margin = 2;

uint32_t abgr_to_argb(uint32_t abgr)
{
  uint32_t b = (abgr >> 16) & 0xFF;
  uint32_t r = abgr & 0xFF;
  return (abgr & 0xFF00FF00) | (r << 16) | b;
}

/* Private forward declarations */
PaletteWindow* PaletteWindow_Create(struct Application*, Palette*);
void PaletteWindow_Init(PaletteWindow*, struct Application*, Palette*);

void PaletteWindow_PreRegister(LPWNDCLASSEX);
void PaletteWindow_PreCreate(LPCREATESTRUCT);

BOOL PaletteWindow_OnCreate(PaletteWindow*, LPCREATESTRUCT);
void PaletteWindow_OnPaint(PaletteWindow*);
void PaletteWindow_OnLButtonUp(PaletteWindow*, int, int);
void PaletteWindow_OnRButtonUp(PaletteWindow*, int, int);
void PaletteWindow_OnContextMenu(PaletteWindow*, int, int);
void PaletteWindow_OnDestroy(PaletteWindow*);
LRESULT CALLBACK PaletteWindow_UserProc(PaletteWindow*, UINT, WPARAM, LPARAM);

void PaletteWindow_DrawSwatch(PaletteWindow*, HDC, int, int, uint32_t);
int PaletteWindow_PosToSwatchIndex(PaletteWindow*, int, int);
void Palette_ColorChangeObserver(void*, uint32_t, uint32_t);

INT_PTR CALLBACK PaletteSettingsDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

PaletteWindow* PaletteWindow_Create(struct Application* app, Palette* palette)
{
  PaletteWindow* window = calloc(1, sizeof(PaletteWindow));

  if (window)
  {
    PaletteWindow_Init(window, app, palette);
  }

  return window;
}

void PaletteWindow_Init(PaletteWindow* window, struct Application* app, Palette* palette)
{
  Window_Init(&window->base, app);

  window->base.szClassName = szClassName;

  window->base.OnCreate = PaletteWindow_OnCreate;
  window->base.OnDestroy = PaletteWindow_OnDestroy;
  window->base.OnPaint = PaletteWindow_OnPaint;
  window->base.PreRegister = PaletteWindow_PreRegister;
  window->base.PreCreate = PaletteWindow_PreCreate;
  window->base.UserProc = PaletteWindow_UserProc;

  window->palette = palette;
}

void PaletteWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
  lpwcex->style = CS_HREDRAW | CS_VREDRAW;
  lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
  lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  lpwcex->lpszClassName = szClassName;
}

BOOL PaletteWindow_OnCreate(PaletteWindow* window, LPCREATESTRUCT lpcs)
{
  UNREFERENCED_PARAMETER(lpcs);

  CheckerConfig checkerCfg = { 0 };
  checkerCfg.tileSize = 8;
  checkerCfg.color[0] = 0x00CCCCCC;
  checkerCfg.color[1] = 0x00FFFFFF;

  HDC hdc = GetDC(window->base.hWnd);
  window->hbrChecker = CreateChecker(&checkerCfg, hdc);
  ReleaseDC(window->base.hWnd, hdc);

  // RegisterColorObserver(Palette_ColorChangeObserver, (void*)window->base.hWnd);
}

void PaletteWindow_OnPaint(PaletteWindow* window)
{
  HWND hwnd = window->base.hWnd;
  PAINTSTRUCT ps;
  HDC hdc;

  /* Get the size of swatch based on settings */
  int swatch_size = g_paletteSettings.swatchSize;

  /* Calculate the number of swatches that can fit in a row */
  RECT rc = { 0 };
  GetClientRect(window->base.hWnd, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);

  /* Ensure there is at least one swatch per row*/
  if (width_indices < 1)
  {
    width_indices = 1;
  }

  /* Begin painting the window */
  hdc = BeginPaint(hwnd, &ps);

  /* Draw the foregroud and background color swatches */
  PaletteWindow_DrawSwatch(window, hdc, 10, 10, (COLORREF)abgr_to_argb(g_color_context.fg_color));
  PaletteWindow_DrawSwatch(window, hdc, 40, 10, (COLORREF)abgr_to_argb(g_color_context.bg_color));

  /* Draw the palette swatches */
  for (size_t i = 0; i < Palette_GetSize(window->palette); ++i)
  {
    PaletteWindow_DrawSwatch(window, hdc,
      10 + ((int)i % width_indices) * (swatch_size + swatch_margin),
      40 + ((int)i / width_indices) * (swatch_size + swatch_margin),
      Palette_At(window->palette, i), i);
  }

  /* End painting the window */
  EndPaint(hwnd, &ps);
}

void PaletteWindow_OnLButtonUp(PaletteWindow* window, int x, int y)
{
  int swatch_size = g_paletteSettings.swatchSize;

  RECT rc = { 0 };
  GetClientRect(window->base.hWnd, &rc);
  int swatch_count = Palette_GetSize(window->palette);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) &&
    ((y - 40) < ((swatch_size + swatch_margin) * height_indices))) {
    int index = PaletteWindow_PosToSwatchIndex(window, x, y);
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.fg_color = abgr_to_argb(Palette_At(window->palette, index));

    RECT rcInvalidate = { 10, 10, 34, 34 };
    InvalidateRect(window->base.hWnd, &rcInvalidate, FALSE);
  }
}

void PaletteWindow_OnRButtonUp(PaletteWindow* window, int x, int y)
{
  int swatch_size = g_paletteSettings.swatchSize;
  Palette* palette = ((struct PanitentApplication*)(window->base.app))->palette;

  RECT rc = { 0 };
  GetClientRect(window->base.hWnd, &rc);
  int swatch_count = Palette_GetSize(palette);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) &&
    ((y - 40) < ((swatch_size + swatch_margin) * height_indices))) {
    int index = PaletteWindow_PosToSwatchIndex(window, x, y);
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.bg_color = abgr_to_argb(Palette_At(palette, index));

    RECT rcInvalidate = { 40, 10, 64, 34 };
    InvalidateRect(window->base.hWnd, &rcInvalidate, FALSE);
  }
}

void PaletteWindow_OnContextMenu(PaletteWindow* window, int x, int y)
{
  x = x < 0 ? 0 : x;
  y = y < 0 ? 0 : y;

  HMENU hMenu = CreatePopupMenu();
  InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_PALETTESETTINGS,
    L"Settings");
  TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, window->base.hWnd, NULL);
}

void PaletteWindow_OnCommand(PaletteWindow* window, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);

  if (LOWORD(wParam) == IDM_PALETTESETTINGS)
  {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PALETTESETTINGS),
        window->base.hWnd, (DLGPROC)PaletteSettingsDlgProc);
  }
}

void PaletteWindow_OnDestroy(PaletteWindow* window)
{
  // RemoveColorObserver(Palette_ColorChangeObserver, (void*)window->base.hWnd);
}

LRESULT CALLBACK PaletteWindow_UserProc(PaletteWindow* window, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {

  case WM_RBUTTONUP:
    PaletteWindow_OnRButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return TRUE;
    break;

  case WM_LBUTTONUP:
    PaletteWindow_OnLButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return TRUE;
    break;

  case WM_CONTEXTMENU:
    PaletteWindow_OnContextMenu(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    break;
  }

  return DefWindowProc(window->base.hWnd, message, wParam, lParam);
}

void PaletteWindow_DrawSwatch(PaletteWindow* window, HDC hdc, int x, int y, uint32_t color)
{
  int swatchSize = g_paletteSettings.swatchSize;

  SetBrushOrgEx(hdc, x, y, NULL);

  RECT rcChecker = {
    x, y, x + swatchSize, y + swatchSize
  };
  FillRect(hdc, &rcChecker, window->hbrChecker);
  // FillRect(hdc, &checkerRc, GetStockObject(BLACK_BRUSH));

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

int PaletteWindow_PosToSwatchIndex(PaletteWindow* window, int x, int y)
{
  int swatch_size = g_paletteSettings.swatchSize;

  RECT rc = {0};
  GetClientRect(window->base.hWnd, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int swatch_outer  = swatch_size + swatch_margin;

  x -= 10;
  y -= 40;

  int index = y / swatch_outer * width_indices + x / swatch_outer;
  printf("Index: %d\n", index);

  return index;
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

void PaletteWindow_PreCreate(LPCREATESTRUCT lpcs)
{
  lpcs->dwExStyle = WS_EX_PALETTEWINDOW;
  lpcs->lpszClass = szClassName;
  lpcs->lpszName = L"Palette";
  lpcs->style = WS_CAPTION | WS_THICKFRAME | WS_VISIBLE;
  lpcs->x = CW_USEDEFAULT;
  lpcs->y = CW_USEDEFAULT;
  lpcs->cx = 300;
  lpcs->cy = 200;
}

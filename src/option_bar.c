#include "option_bar.h"
#include "panitent.h"
#include <commctrl.h>
#include <windowsx.h>
#include "primitives_context.h"
#include "wu_primitives.h"
#include "bresenham.h"
#include "swatch.h"

OptionBar g_option_bar;

#define IDCB_STENCIL_ALGORITHM 1553
#define IDCB_THICKNESS 1554

void OptionBar_OnCommand(WPARAM wparam, LPARAM lparam)
{
  if (HIWORD(wparam) != LBN_SELCHANGE)
    return;

  switch (LOWORD(wparam))
  {
    case IDCB_STENCIL_ALGORITHM:
      switch (ComboBox_GetCurSel((HWND)lparam))
      {
        case 0:
          g_primitives_context = g_bresenham_primitives;
          break;
        case 1:
          g_primitives_context = g_wu_primitives;
          break;
      }
      break;
    case IDCB_THICKNESS:
      SetThickness(ComboBox_GetCurSel((HWND)lparam) + 1);
      break;
  }

  if (LOWORD(wparam) == IDCB_STENCIL_ALGORITHM &&
      HIWORD(wparam) == LBN_SELCHANGE) {
    switch (ComboBox_GetCurSel((HWND)lparam)) {
      break;
    default:
      break;
    }
  }
}

LRESULT CALLBACK OptionBar_WndProc(HWND hwnd, UINT message, WPARAM wparam,
    LPARAM lparam)
{
  switch (message) {
  case WM_CREATE:
  {
    HWND hcombo = CreateWindowEx(0,
                                 WC_COMBOBOX,
                                 L"",
                                 CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD |
                                     WS_OVERLAPPED | WS_VISIBLE,
                                 64,
                                 3,
                                 100,
                                 20,
                                 hwnd,
                                 (HMENU)IDCB_STENCIL_ALGORITHM,
                                 GetModuleHandle(NULL),
                                 NULL);
    SetGuiFont(hcombo);

    ComboBox_AddString(hcombo, L"Bresenham");
    ComboBox_AddString(hcombo, L"Wu");

    HWND hComboThickness= CreateWindowEx(0, WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED |
        WS_VISIBLE,
        180, 3, 100, 20,
        hwnd, (HMENU)IDCB_THICKNESS, GetModuleHandle(NULL), NULL);
    SetGuiFont(hComboThickness);

    WCHAR szThickness[4];
    for (size_t i = 1; i <= 10; ++i)
    {
      _itow_s(i, szThickness, 4, 10);
      ComboBox_AddString(hComboThickness, szThickness);
    }

    HWND hedit =
        CreateWindowEx(0,
                       WC_EDIT,
                       L"Sample Text",
                       WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                       296,
                       3,
                       140,
                       20,
                       hwnd,
                       NULL,
                       GetModuleHandle(NULL),
                       NULL);
    SetGuiFont(hedit);
    g_option_bar.textstring_handle = hedit;

  } break;
  case WM_COMMAND:
    OptionBar_OnCommand(wparam, lparam);
    break;
  default:
    return DefWindowProc(hwnd, message, wparam, lparam);
    break;
  }

  return 0;
}

void OptionBar_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex    = {0};
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.lpfnWndProc   = (WNDPROC)OptionBar_WndProc;
  wcex.hInstance     = hInstance;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.lpszClassName = L"Win32Class_OptionBar";

  ATOM class_atom = RegisterClassEx(&wcex);
  if (!class_atom) {
    MessageBox(NULL,
               L"Failed to register option bar class!",
               NULL,
               MB_OK | MB_ICONERROR);
    return;
  }

  g_option_bar.win_class = class_atom;
}

void OptionBar_Create(HWND hwnd)
{
  HWND handle = CreateWindowEx(0,
                               MAKEINTATOM(g_option_bar.win_class),
                               NULL,
                               WS_CHILD | WS_VISIBLE,
                               0,
                               0,
                               500,
                               64,
                               hwnd,
                               NULL,
                               GetModuleHandle(NULL),
                               NULL);

  if (!handle) {
    MessageBox(NULL,
               L"Failed to create option bar window!",
               NULL,
               MB_OK | MB_ICONERROR);
    return;
  }

  g_option_bar.win_handle = handle;
}

#include "precomp.h"
#include <windowsx.h>
#include <commctrl.h>

#include <shlwapi.h>
#include <stdio.h>

#include "new.h"
#include "debug.h"
#include "document.h"
#include "viewport.h"
#include "color_context.h"

void RegisterNewFileDialog()
{
  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)NewFileDialogWndProc;
  wc.hInstance     = GetModuleHandle(NULL);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = L"NewFileDialogClass";
  if (!RegisterClass(&wc))
    printf("[NewFileDialog] Class registration failed\n");
}

extern Viewport g_viewport;

HWND hEditWidth;
HWND hEditHeight;
HWND hComboBackground;
extern HWND hButtonOk;

LRESULT CALLBACK NewFileDialogWndProc(HWND hwnd,
                                      UINT msg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
  switch (msg) {
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDB_OK:
    {
      WCHAR szWidth[40];
      WCHAR szHeight[40];
      int iWidth;
      int iHeight;

      GetWindowText(hEditWidth, szWidth, 40);
      GetWindowText(hEditHeight, szHeight, 40);

      COLORREF bgColor;
      int bgIndex = ComboBox_GetCurSel(hComboBackground);

      switch (bgIndex)
      {
        case 0:
          bgColor = 0x00000000;
          break;
        case 1:
          bgColor = 0xFFFFFFFF;
          break;
        case 2:
          bgColor = 0xFF000000;
          break;
        case 3:
          bgColor = g_color_context.fg_color;
          break;
        case 4:
          bgColor = g_color_context.bg_color;
          break;
      }

      iWidth  = StrToInt(szWidth);
      iHeight = StrToInt(szHeight);

      printf("[NewFile] width: %d, height: %d\n", iWidth, iHeight);

      DestroyWindow(hwnd);
      Document_New(iWidth, iHeight);

      if (g_viewport.document && g_viewport.document->canvas)
      {
        Canvas_FillSolid(g_viewport.document->canvas, bgColor);
      }
    } break;
    default:
      break;
    }
    break;
  case WM_SIZE:
  {
    WORD cx = LOWORD(lParam);
    WORD cy = HIWORD(lParam);
    SetWindowPos(hButtonOk, NULL, cx - 110, cy - 40, 100, 30, SWP_NOZORDER);
  } break;
  case WM_CREATE:
  {
    HWND hStaticWidth = CreateWindow(L"STATIC",
                                     L"Width:",
                                     WS_CHILD | WS_VISIBLE,
                                     10,
                                     15,
                                     70,
                                     23,
                                     hwnd,
                                     NULL,
                                     GetModuleHandle(NULL),
                                     NULL);
    SetGuiFont(hStaticWidth);

    hEditWidth = CreateWindowEx(WS_EX_CLIENTEDGE,
                                L"EDIT",
                                L"640",
                                WS_CHILD | WS_VISIBLE | ES_NUMBER,
                                100,
                                10,
                                100,
                                23,
                                hwnd,
                                NULL,
                                GetModuleHandle(NULL),
                                NULL);
    SetGuiFont(hEditWidth);

    HWND hStaticHeight = CreateWindow(L"STATIC",
                                      L"Height:",
                                      WS_CHILD | WS_VISIBLE,
                                      10,
                                      42,
                                      70,
                                      23,
                                      hwnd,
                                      NULL,
                                      GetModuleHandle(NULL),
                                      NULL);
    SetGuiFont(hStaticHeight);

    hEditHeight = CreateWindowEx(WS_EX_CLIENTEDGE,
                                 L"EDIT",
                                 L"480",
                                 WS_CHILD | WS_VISIBLE | ES_NUMBER,
                                 100,
                                 38,
                                 100,
                                 23,
                                 hwnd,
                                 NULL,
                                 GetModuleHandle(NULL),
                                 NULL);
    SetGuiFont(hEditHeight);

    HWND hStaticBackground = CreateWindow(L"STATIC", L"Background:",
        WS_CHILD | WS_VISIBLE,
        10, 69, 70, 23,
        hwnd, NULL, GetModuleHandle(NULL), NULL);
    SetGuiFont(hStaticBackground);

    hComboBackground = CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX,
        L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED |
            WS_VISIBLE,
        100, 66, 100, 23,
        hwnd, NULL, GetModuleHandle(NULL), NULL);
    SetGuiFont(hComboBackground);
    ComboBox_AddString(hComboBackground, L"Transparent");
    ComboBox_AddString(hComboBackground, L"White");
    ComboBox_AddString(hComboBackground, L"Black");
    ComboBox_AddString(hComboBackground, L"Primary color");
    ComboBox_AddString(hComboBackground, L"Secondary color");
    ComboBox_SetCurSel(hComboBackground, 1);

    hButtonOk = CreateWindow(L"BUTTON",
                             L"OK",
                             WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                             200,
                             250,
                             100,
                             30,
                             hwnd,
                             (HMENU)IDB_OK,
                             GetModuleHandle(NULL),
                             NULL);
    SetGuiFont(hButtonOk);
  } break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
    break;
  }

  return 0;
}

void NewFileDialog(HWND hwnd)
{
  RegisterNewFileDialog();

  RECT rc   = {0};
  rc.right  = 210;
  rc.bottom = 170;
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

  CreateWindow(L"NewFileDialogClass",
               L"New",
               WS_VISIBLE | WS_OVERLAPPEDWINDOW,
               (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2,
               (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2,
               rc.right - rc.left,
               rc.bottom - rc.top,
               hwnd,
               NULL,
               GetModuleHandle(NULL),
               NULL);
}

#include "precomp.h"
#include "resource.h"
#include "welcome.h"
#include "panitent.h"
#include <commctrl.h>
#include "new.h"

#define IDB_FILE_NEW 1277

const WCHAR szWelcomeWndClass[] = L"Win32Class_Welcome";

static void OnCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  HWND hButton = CreateWindowEx(0, WC_BUTTON, L"New",
      WS_CHILD | WS_VISIBLE, 10, 96, 200, 30, hwnd, (HMENU)IDB_FILE_NEW,
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
  SetGuiFont(hButton);

  HWND hButton2 = CreateWindowEx(0, WC_BUTTON, L"Open",
      WS_CHILD | WS_VISIBLE, 10, 130, 200, 30, hwnd, NULL,
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
  SetGuiFont(hButton2);
}

static void OnPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps = {0};
  HDC hdc;

  hdc = BeginPaint(hwnd, &ps);

  HDC hBmDc = CreateCompatibleDC(hdc);
  HBITMAP hbmSplash = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH));
  HGDIOBJ hOldObj = SelectObject(hBmDc, hbmSplash);
    
  TransparentBlt(hdc, 10, 10, 180, 64, hBmDc, 0, 0, 180, 64, RGB(255, 0, 255));

  SelectObject(hBmDc, hOldObj);
  EndPaint(hwnd, &ps);
}

static void OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDB_FILE_NEW)
  {
    NewFileDialog(NULL);
  }
}

LRESULT CALLBACK Welcome_WndProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      OnCreate(hwnd, wParam, lParam);
      return 0;
    case WM_PAINT:
      OnPaint(hwnd, wParam, lParam);
      return 0;
    case WM_COMMAND:
      OnCommand(hwnd, wParam, lParam);
      return 0;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL Welcome_Register(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {0};
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)Welcome_WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = szWelcomeWndClass;

  return RegisterClassEx(&wcex);
}

HWND Welcome_Create(HWND hwndParent, LPWSTR lpszCaption)
{
  HWND hwnd = CreateWindowEx(0, szWelcomeWndClass, lpszCaption,
      WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 640, 480, hwndParent, NULL,
      GetModuleHandle(NULL), NULL);
  return hwnd;
}

#include <assert.h>
#include <stdint.h>
#include <commctrl.h>
#include <windowsx.h>
#include "precomp.h"
#include "resource.h"
#include "dockhost.h"

typedef struct _main_window {
  ATOM wndclass;
  HWND hwnd;
} main_window_t;

main_window_t g_main_window;

void main_window_onpaint(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
  PAINTSTRUCT ps;
  HDC hdc;
  hdc = BeginPaint(hwnd, &ps);

  EndPaint(hwnd, &ps);
}

HWND hDockHost;

LRESULT CALLBACK main_window_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
    case WM_CREATE:
      hDockHost = DockHost_Create(hwnd);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_SIZE:
      SetWindowPos(hDockHost, NULL, 0, 0, LOWORD(lparam), HIWORD(lparam), SWP_NOMOVE);
      break;
    case WM_PAINT:
      main_window_onpaint(hwnd, wparam, lparam);
      break;
    default:
      return DefWindowProc(hwnd, message, wparam, lparam);
      break;
  }

  return 0;
}

void main_window_register_class(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)main_window_wndproc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, IDI_ICON);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_MainWindow";
  wcex.lpszMenuName = NULL;
  wcex.hIconSm = NULL;

  ATOM class_atom = RegisterClassEx(&wcex);
  assert(class_atom);
  g_main_window.wndclass = class_atom;
}

void main_window_create()
{
  HWND handle = CreateWindowEx(0, MAKEINTATOM(g_main_window.wndclass), L"DockHost",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL); 

  assert(handle);

  g_main_window.hwnd = handle;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  DockHost_Register(hInstance);
  main_window_register_class(hInstance);
  main_window_create();
  ShowWindow(g_main_window.hwnd, nCmdShow);

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

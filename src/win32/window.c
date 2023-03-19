#include "window.h"

static const WCHAR szClassName[] = L"DummyWindowClass";

Window* Window_Create(struct Application*);
void Window_Init(Window*, struct Application*);

void Window_PreRegister(LPWNDCLASSEX);
void Window_PreCreate(LPCREATESTRUCT);

BOOL Window_OnCreate(Window*, LPCREATESTRUCT);
void Window_OnPaint(Window*);
void Window_OnDestroy(Window*);
LRESULT CALLBACK Window_UserProc(Window*, UINT, WPARAM, LPARAM);

LRESULT CALLBACK Window_WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL Window_Register(Window*);
void Window_CreateWindow(Window*, HWND);

Window* Window_Create(struct Application* app)
{
  Window* window = calloc(1, sizeof(Window));

  if (window) {
    Window_Init(window, app);
  }

  return window;
}

void Window_Init(Window* window, struct Application* app)
{
  window->app = app;

  window->szClassName = szClassName;

  window->PreRegister = (void(*)(LPWNDCLASSEX))Window_PreRegister;
  window->PreCreate = (void(*)(LPCREATESTRUCT))Window_PreCreate;

  window->OnCreate = (BOOL(*)(Window*, LPCREATESTRUCT))Window_OnCreate;
  window->OnPaint = (void (*)(Window*))Window_OnPaint;
  window->OnDestroy = (void (*)(Window*))Window_OnDestroy;

  window->UserProc = (LRESULT (CALLBACK *)(Window*, UINT, WPARAM, LPARAM))Window_UserProc;
}

void Window_PreRegister(LPWNDCLASSEX lpwcex)
{
  lpwcex->cbSize = sizeof(WNDCLASSEX);
  lpwcex->style = CS_HREDRAW | CS_VREDRAW;
  lpwcex->lpfnWndProc = (WNDPROC)Window_WndProc;
  lpwcex->cbWndExtra = sizeof(Window*);
  lpwcex->hInstance = GetModuleHandle(NULL);
  lpwcex->hIcon = LoadIcon(NULL, IDI_APPLICATION);
  lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
  lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  lpwcex->lpszClassName = szClassName;
}

void Window_PreCreate(LPCREATESTRUCT lpcs)
{
  lpcs->dwExStyle = 0;
  lpcs->lpszClass = szClassName;
  lpcs->lpszName = L"Dummy Window";
  lpcs->style = WS_OVERLAPPEDWINDOW;
  lpcs->x = CW_USEDEFAULT;
  lpcs->y = CW_USEDEFAULT;
  lpcs->cx = CW_USEDEFAULT;
  lpcs->cy = CW_USEDEFAULT;
  lpcs->hwndParent = NULL;
  lpcs->hInstance = GetModuleHandle(NULL);
  lpcs->lpCreateParams = (LPVOID)NULL;
}

BOOL Window_OnCreate(Window* window, LPCREATESTRUCT lpcs)
{
  return TRUE;
}

void Window_OnPaint(Window* window)
{
  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(window->hWnd, &ps);

  RECT rcClient = { 0 };
  GetClientRect(window->hWnd, &rcClient);

  SetBkMode(hdc, TRANSPARENT);

  const WCHAR szDummyText[] = L"This is a dummy window created by passing default overloads";
  size_t nDummyTextLen = 0;
  StringCchLength(szDummyText, STRSAFE_MAX_CCH, &nDummyTextLen);

  DrawTextEx(hdc, szDummyText, nDummyTextLen, &rcClient, DT_SINGLELINE | DT_CENTER | DT_VCENTER, NULL);

  EndPaint(window->hWnd, &ps);
}

void Window_OnDestroy(Window* window)
{
  PostQuitMessage(0);
}

LRESULT CALLBACK Window_UserProc(Window* window, UINT message, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProc(window->hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Window_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window* window = NULL;

  if (message == WM_CREATE || message == WM_NCCREATE)
  {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
    window = (Window*)lpcs->lpCreateParams;
    window->hWnd = hWnd;
    SetWindowLongPtr(hWnd, 0, window);
  }
  else
  {
    window = (Window*)GetWindowLongPtr(hWnd, 0);
  }

  switch (message)
  {

  case WM_CREATE:
    return window->OnCreate(window, (LPCREATESTRUCT)lParam);

  case WM_PAINT:
    window->OnPaint(window);
    return 0;

  case WM_DESTROY:
    window->OnDestroy(window);
    return 0;
  }

  if (window) {
    return window->UserProc(window, message, wParam, lParam);
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL Window_Register(Window* window)
{
  WNDCLASSEX wcex = { 0 };
  BOOL bRegistered = FALSE;

  bRegistered = GetClassInfoEx(window->app->hInstance, window->szClassName, &wcex);

  if (!bRegistered && window->PreRegister)
  {
    window->PreRegister(&window->wcex);
    window->wcex.cbSize = sizeof(WNDCLASSEX);
    window->wcex.lpfnWndProc = (WNDPROC)Window_WndProc;
    window->wcex.cbWndExtra = sizeof(Window*);
    window->wcex.hInstance = window->app->hInstance;
    return (BOOL)RegisterClassEx(&window->wcex);
  }

  return TRUE;
}

void Window_CreateWindow(Window* window, HWND hParent)
{
  Window_Register(window);

  LPWNDCLASSEX lpwcex = &window->wcex;
  LPCREATESTRUCT cs = &window->cs;

  window->PreCreate(cs);
  cs->lpCreateParams = (LPVOID)window;

  if (lpwcex->lpszClassName)
  {
    cs->lpszClass = window->wcex.lpszClassName;
  }

  if (hParent)
  {
    cs->style &= ~(WS_CAPTION | WS_THICKFRAME);
    cs->style |= WS_CHILD;
  }

  HWND hWindow = CreateWindowEx(
    cs->dwExStyle,
    cs->lpszClass,
    cs->lpszName,
    cs->style,
    cs->x,
    cs->y,
    cs->cx,
    cs->cy,
    hParent,
    cs->hMenu,
    lpwcex->hInstance,
    (LPVOID)window);

  window->hWnd = hWindow;

  ShowWindow(hWindow, SW_NORMAL);
  UpdateWindow(hWindow);
}

void Window_SetTheme(Window* window, PCWSTR lpszThemeName)
{
  SetWindowTheme(window->hWnd, lpszThemeName, NULL);
}

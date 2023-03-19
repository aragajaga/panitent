#include "win32/application.h"
#include "test_window.h"

#include "resource.h"

static const WCHAR szClassName[] = L"TestWindowClass";

TestWindow* TestWindow_Create(Application*);
void TestWindow_Init(TestWindow* window, Application*);

void TestWindow_PreRegister(LPWNDCLASSEX);
void TestWindow_PreCreate(LPCREATESTRUCT);

BOOL TestWindow_OnCreate(TestWindow*, LPCREATESTRUCT);
void TestWindow_OnSize(TestWindow*, UINT, int, int);
void TestWindow_OnPaint(TestWindow*);
LRESULT CALLBACK TestWindow_UserProc(struct Window*, UINT message, WPARAM wParam, LPARAM lParam);

TestWindow* TestWindow_Create(Application* app)
{
  TestWindow* window = calloc(1, sizeof(TestWindow));

  if (window)
  {
    TestWindow_Init(window, app);
  }

  return window;
}

void TestWindow_Init(TestWindow* window, Application* app)
{
  Window_Init(&window->base, app);

  window->base.szClassName = szClassName;

  window->base.PreRegister = TestWindow_PreRegister;
  window->base.PreCreate = TestWindow_PreCreate;

  window->base.OnCreate = TestWindow_OnCreate;
  window->base.OnPaint = TestWindow_OnPaint;
  window->base.UserProc = TestWindow_UserProc;

  TreeViewCtl_Init(&window->treeView, app);
}

void TestWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
  lpwcex->style = CS_HREDRAW | CS_VREDRAW;
  lpwcex->hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
  lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
  lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  lpwcex->lpszClassName = szClassName;
}

void TestWindow_PreCreate(LPCREATESTRUCT lpcs)
{
  lpcs->lpszClass = szClassName;
  lpcs->lpszName = L"Panit.ent Framework Test";
  lpcs->style = WS_OVERLAPPEDWINDOW;
  lpcs->x = CW_USEDEFAULT;
  lpcs->y = CW_USEDEFAULT;
  lpcs->cx = CW_USEDEFAULT;
  lpcs->cy = CW_USEDEFAULT;
}

BOOL TestWindow_OnCreate(TestWindow* window, LPCREATESTRUCT lpcs)
{
  Window_CreateWindow(&window->treeView, window->base.hWnd);
  TreeViewCtl_InsertItem(&window->treeView, L"Palette", -1, NULL, NULL, NULL);
  TreeViewCtl_InsertItem(&window->treeView, L"Canvas", -1, NULL, NULL, NULL);
  TreeViewCtl_InsertItem(&window->treeView, L"Viewport", -1, NULL, NULL, NULL);
  // TreeViewCtl_SetItemHeight(&window->treeView, 32);
  Window_SetTheme(&window->treeView, L"explorer");

  return TRUE;
}

void TestWindow_OnSize(TestWindow* window, UINT state, int x, int y)
{
  SetWindowPos(window->treeView.base.hWnd, NULL, 10, 10, 200, y - 20, 0);
}

void TestWindow_OnPaint(TestWindow* window)
{
  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(window->base.hWnd, &ps);

  EndPaint(window->base.hWnd, &ps);
}

LRESULT CALLBACK TestWindow_UserProc(TestWindow* window, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {

  case WM_SIZE:
    TestWindow_OnSize(window, (UINT)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
    return 0;
    break;
  }

  return DefWindowProc(window->base.hWnd, message, wParam, lParam);
}

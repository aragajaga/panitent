#include "../src/win32/application.h"
#include "test_window.h"

#include "test_application.h"
#include "../src/palette_window.h"

static const WCHAR szClassName[] = L"TestWindowClass";

TestWindow* TestWindow_Create(struct Application* app);
void TestWindow_Init(TestWindow*, Application*);

void TestWindow_PreRegister(LPWNDCLASSEX);
void TestWindow_PreCreate(LPCREATESTRUCT);

BOOL TestWindow_OnCreate(TestWindow*, LPCREATESTRUCT);
void TestWindow_OnSize(TestWindow*, UINT, int, int);
void TestWindow_OnPaint(TestWindow*);
LRESULT CALLBACK TestWindow_UserProc(struct Window*, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

TestWindow* TestWindow_Create(struct Application* app)
{
    TestWindow* pTestWindow = (TestWindow*)malloc(sizeof(TestWindow));
    if (pTestWindow)
    {
        memset(pTestWindow, 0, sizeof(TestWindow));
        TestWindow_Init(pTestWindow, app);
    }

    return pTestWindow;
}

void TestWindow_Init(TestWindow* window, struct Application* app)
{
    Window_Init(&window->base, app);

    window->base.szClassName = szClassName;

    window->base.PreRegister = TestWindow_PreRegister;
    window->base.PreCreate = TestWindow_PreCreate;
    window->base.OnCreate = TestWindow_OnCreate;
    window->base.OnPaint = TestWindow_OnPaint;
    window->base.UserProc = TestWindow_UserProc;

    window->paletteWindow = PaletteWindow_Create(window->base.app, ((struct TestApplication*)app)->palette);
}

LRESULT CALLBACK TestWindow_UserProc(struct Window* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        TestWindow_OnSize(window, (UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        return 0;
        break;
    }

    return Window_UserProcDefault(window, hWnd, message, wParam, lParam);
}

void TestWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void TestWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    // lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"panitent_framework_test";
    lpcs->style = WS_OVERLAPPEDWINDOW;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = CW_USEDEFAULT;
    lpcs->cy = CW_USEDEFAULT;
}

BOOL TestWindow_OnCreate(TestWindow* window, LPCREATESTRUCT lpcs)
{
    Window_CreateWindow(window->paletteWindow, window->base.hWnd);
}

void TestWindow_OnSize(TestWindow* window, UINT state, int x, int y)
{
    SetWindowPos(window->paletteWindow->base.hWnd, NULL, 0, 0, x, y, SWP_NOREPOSITION);
}

void TestWindow_OnPaint(TestWindow* window)
{
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(window->base.hWnd, &ps);

    EndPaint(window->base.hWnd, &ps);
}

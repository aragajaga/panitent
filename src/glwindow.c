#include "precomp.h"

#include "win32/window.h"
#include "glwindow.h"
#include "resource.h"
#include "panitent.h"

#include <gl/GL.h>

static const WCHAR szClassName[] = L"GLWindowClass";

/* Private forward declarations */
GLWindow* GLWindow_Create(struct Application*);
void GLWindow_Init(GLWindow*, struct Application*);

void GLWindow_PreRegister(LPWNDCLASSEX);
void GLWindow_PreCreate(LPCREATESTRUCT);

BOOL GLWindow_OnCreate(GLWindow*, LPCREATESTRUCT);
void GLWindow_OnPaint(GLWindow*);
void GLWindow_OnLButtonUp(GLWindow*, int, int);
void GLWindow_OnRButtonUp(GLWindow*, int, int);
void GLWindow_OnContextMenu(GLWindow*, int, int);
void GLWindow_OnDestroy(GLWindow*);
LRESULT CALLBACK GLWindow_UserProc(GLWindow*, HWND hWnd, UINT, WPARAM, LPARAM);

GLWindow* GLWindow_Create(struct Application* app)
{
    GLWindow* pGLWindow = (GLWindow*)malloc(sizeof(GLWindow));
    memset(pGLWindow, 0, sizeof(GLWindow));

    if (pGLWindow)
    {
        GLWindow_Init(pGLWindow, app);
    }

    return pGLWindow;
}

void GLWindow_Init(GLWindow* pGLWindow, struct Application* app)
{
    Window_Init(&pGLWindow->base, app);

    pGLWindow->base.szClassName = szClassName;

    pGLWindow->base.OnCreate = (FnWindowOnCreate)GLWindow_OnCreate;
    pGLWindow->base.OnDestroy = (FnWindowOnDestroy)GLWindow_OnDestroy;
    pGLWindow->base.OnPaint = (FnWindowOnPaint)GLWindow_OnPaint;
    pGLWindow->base.PreRegister = (FnWindowPreRegister)GLWindow_PreRegister;
    pGLWindow->base.PreCreate = (FnWindowPreCreate)GLWindow_PreCreate;
    pGLWindow->base.UserProc = (FnWindowUserProc)GLWindow_UserProc;
}

void GLWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

BOOL GLWindow_OnCreate(GLWindow* pGLWindow, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(pGLWindow);
    UNREFERENCED_PARAMETER(lpcs);

    PIXELFORMATDESCRIPTOR pfd;
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 0;
    pfd.cRedShift = 0;
    pfd.cGreenBits = 0;
    pfd.cGreenShift = 0;
    pfd.cBlueBits = 0;
    pfd.cBlueShift = 0;
    pfd.cAlphaBits = 0;
    pfd.cAlphaShift = 0;
    pfd.cAccumBits = 0;
    pfd.cAccumRedBits = 0;
    pfd.cAccumGreenBits = 0;
    pfd.cAccumBlueBits = 0;
    pfd.cAccumAlphaBits = 0;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.bReserved = 0;
    pfd.dwLayerMask = 0;
    pfd.dwVisibleMask = 0;
    pfd.dwDamageMask = 0;

    HDC ourWindowHandleToDeviceContext = GetDC(pGLWindow->base.hWnd);

    int letWindowsChooseThisPixelFormat;
    letWindowsChooseThisPixelFormat = ChoosePixelFormat(ourWindowHandleToDeviceContext, &pfd);

    SetPixelFormat(ourWindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);

    HGLRC ourOpenGLRenderingContext = wglCreateContext(ourWindowHandleToDeviceContext);
    wglMakeCurrent(ourWindowHandleToDeviceContext, ourOpenGLRenderingContext);

    // MessageBoxA(NULL, (char *)glGetString(GL_VERSION), "OPENGL VERSION", 0);

    wglDeleteContext(ourOpenGLRenderingContext);
}

void GLWindow_OnPaint(GLWindow* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    SwapBuffers(hdc);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void GLWindow_OnLButtonUp(GLWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void GLWindow_OnRButtonUp(GLWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void GLWindow_OnContextMenu(GLWindow* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void GLWindow_OnCommand(GLWindow* window, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
}

void GLWindow_OnDestroy(GLWindow* window)
{
    UNREFERENCED_PARAMETER(window);
}

LRESULT CALLBACK GLWindow_UserProc(GLWindow* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {

    case WM_RBUTTONUP:
        GLWindow_OnRButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONUP:
        GLWindow_OnLButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        GLWindow_OnContextMenu(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault(window, hWnd, message, wParam, lParam);
}

void GLWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = WS_EX_PALETTEWINDOW;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"GLWindow";
    lpcs->style = WS_CAPTION | WS_THICKFRAME | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

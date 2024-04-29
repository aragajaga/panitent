#ifndef _WIN32_WINDOW_H_INCLUDED
#define _WIN32_WINDOW_H_INCLUDED

#include "common.h"
#include "application.h"

typedef struct Window Window;

typedef void (*FnWindowPreRegister)(LPWNDCLASSEX lpwcex);
typedef void (*FnWindowPreCreate)(LPCREATESTRUCT lpcs);
typedef BOOL (*FnWindowOnCreate)(Window* pWindow, LPCREATESTRUCT lpcs);
typedef void (*FnWindowOnPaint)(Window* pWindow);
typedef BOOL (*FnWindowOnClose)(Window* pWindow);
typedef void (*FnWindowOnDestroy)(Window* pWindow);
typedef BOOL (*FnWindowOnCommand)(Window* pWindow, WPARAM wParam, LPARAM lParam);
typedef void (*FnWindowOnSize)(Window* pWindow, UINT state, int x, int y);
typedef void (*FnWindowPostCreate)(Window* pWindow);
typedef LRESULT (*FnWindowUserProc)(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct Window {
  struct Application* app;
  HWND hWnd;
  PCWSTR szClassName;
  WNDCLASSEX wcex;
  CREATESTRUCT cs;
  WNDPROC pfnPrevWindowProc;

  FnWindowPreRegister PreRegister;
  FnWindowPreCreate PreCreate;
  FnWindowOnCreate OnCreate;
  FnWindowOnPaint OnPaint;
  FnWindowOnClose OnClose;
  FnWindowOnDestroy OnDestroy;
  FnWindowOnCommand OnCommand;
  FnWindowOnSize OnSize;
  FnWindowPostCreate PostCreate;
  FnWindowUserProc UserProc;
};

Window* Window_Create(struct Application*);
void Window_Attach(Window* pWindow, HWND hWnd);
void Window_Init(Window*, struct Application*);

void Window_PreRegister(LPWNDCLASSEX lpcs);
void Window_PreCreate(LPCREATESTRUCT lpcs);

BOOL Window_OnCreate(Window* pWindow, LPCREATESTRUCT lpcs);
void Window_PostCreate(Window* pWindow);
void Window_OnPaint(Window* pWindow);
BOOL Window_OnCommand(Window* pWindow, WPARAM wParam, LPARAM lparam);
void Window_OnSize(Window* pWindow, int x, int y);
BOOL Window_OnCommand(Window* pWindow, WPARAM wParam, LPARAM lParam);
BOOL Window_OnClose(Window* pWindow);
void Window_OnDestroy(Window* pWindow);
LRESULT CALLBACK Window_UserProc(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT Window_UserProcDefault(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Window_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL Window_Register(Window* pWindow);
HWND Window_CreateWindow(Window* pWindow, HWND hWndParent);
void Window_SetTheme(Window* pWindow, PCWSTR lpszThemeName);
DWORD Window_GetStyle(Window* pWindow);
LRESULT Window_SendMessage(Window* pWindow, UINT message, WPARAM wParam, LPARAM lParam);
void Window_SetStyle(Window* pWindow, DWORD dwStyle);
HWND Window_GetHWND(Window* pWindow);
void Window_ApplyUIFont(Window* pWindow);
void Window_GetClientRect(Window* pWindow, LPRECT lpcs);
void Window_Invalidate(Window* pWindow);
BOOL Window_Show(Window* pWindow, int nCmdShow);

void _WindowInitHelper_SetPreRegisterRoutine(Window* pWindow, void(*pfnPreRegister)(LPWNDCLASSEX lpwcex));
void _WindowInitHelper_SetPreCreateRoutine(Window* pWindow, void(*pfnPreCreate)(LPCREATESTRUCT lpcs));
void _WindowInitHelper_SetUserProcRoutine(Window* pWindow, void(*pfnUserProc)(Window* pWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam));

int Win32_Rect_GetWidth(LPRECT lprc);
int Win32_Rect_GetHeight(LPRECT lprc);

#endif  /* _WIN32_WINDOW_H_INCLUDED */

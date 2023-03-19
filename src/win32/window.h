#ifndef _WIN32_WINDOW_H_INCLUDED
#define _WIN32_WINDOW_H_INCLUDED

#include "common.h"
#include "application.h"

typedef struct Window Window;

struct Window {
  struct Application* app;
  HWND hWnd;
  PCWSTR szClassName;
  WNDCLASSEX wcex;
  CREATESTRUCT cs;
  void (*PreRegister)(LPWNDCLASSEX);
  void (*PreCreate)(LPCREATESTRUCT);
  BOOL (*OnCreate)(Window*, LPCREATESTRUCT);
  void (*OnPaint)(Window*);
  void (*OnDestroy)(Window*);
  LRESULT (CALLBACK *UserProc)(Window*, UINT, WPARAM, LPARAM);
};

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
void Window_SetTheme(Window*, PCWSTR);

#endif  /* _WIN32_WINDOW_H_INCLUDED */

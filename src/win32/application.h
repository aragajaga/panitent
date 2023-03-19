#ifndef _WIN32_APPLICATION_INCLUDED
#define _WIN32_APPLICATION_INCLUDED

#include "common.h"

typedef struct Application Application;

struct Application {
  HINSTANCE hInstance;
};

LRESULT CALLBACK Application_WndProc(HWND, UINT, WPARAM, LPARAM);
void Application_Init(Application*);
int Application_Run(Application*);

#endif  /* _WIN32_APPLICATION_INCLUDED */

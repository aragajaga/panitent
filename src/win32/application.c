#include "application.h"

static const WCHAR szClassName[] = L"TestWindowClass";

Application* Application_Create();
void Application_Init(Application*);

int Application_Run(Application*);

Application* Application_Create()
{
  Application* app = (Application*)malloc(sizeof(Application));
  memset(app, 0, sizeof(Application));
  Application_Init(app);
  return app;
}

void Application_Init(Application* app)
{
  INITCOMMONCONTROLSEX iccex = { 0 };
  iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  iccex.dwICC = ICC_TREEVIEW_CLASSES;
  InitCommonControlsEx(&iccex);

  app->hInstance = GetModuleHandle(NULL);
}

int Application_Run(Application* app)
{
  MSG msg = { 0 };

  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int) msg.wParam;
}

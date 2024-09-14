#include "application.h"

static const WCHAR szClassName[] = L"TestWindowClass";

Application* Application_Create();
void Application_Init(Application*);

int Application_Run(Application*);

void Application_InitCommonControls()
{
    INITCOMMONCONTROLSEX iccex = { 0 };
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&iccex);
}

Application* Application_Create()
{
    Application* pApplication = (Application*)malloc(sizeof(Application));
    
    if (pApplication)
    {
        Application_Init(pApplication);
    }
    
    return pApplication;
}

void Application_Init(Application* pApplication)
{
    memset(pApplication, 0, sizeof(Application));

    Application_InitCommonControls();

    pApplication->hInstance = GetModuleHandle(NULL);
}

int Application_Run(Application* app)
{
    MSG msg = { 0 };

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

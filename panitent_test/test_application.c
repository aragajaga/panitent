#include "../src/win32/application.h"
#include "../src/win32/window.h"
#include "test_application.h"
#include "test_window.h"
#include <string.h>

void TestApplication_Init(TestApplication* app)
{
    Application_Init(&app->base);

    app->palette = Palette_Create();
    app->mainWindow = TestWindow_Create(app);
}

TestApplication* TestApplication_Create()
{
    TestApplication* pTestApplication = (TestApplication*)malloc(sizeof(struct TestApplication));

    if (pTestApplication)
    {
        memset(pTestApplication, 0, sizeof(TestApplication));
        TestApplication_Init(pTestApplication);
    }

    return pTestApplication;
}

int TestApplication_Run(TestApplication* pTestApplication)
{
    // Application_Run(app);

    HWND hWindow = Window_CreateWindow((Window*)pTestApplication->mainWindow, NULL);
    Window_Show(pTestApplication->mainWindow, SW_SHOW);

    return Application_Run(pTestApplication);
}

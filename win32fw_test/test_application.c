#include "win32/application.h"
#include "win32/window.h"

#include "test_application.h"
#include "test_window.h"

void TestApplication_Init(TestApplication* app)
{
  Application_Init(&app->application);

  app->mainWindow = TestWindow_$new(app);
}

TestApplication* TestApplication_Create()
{
  TestApplication* pTestApplication = (TestApplication*)malloc(sizeof(TestApplication));
  memset(pTestApplication, 0, sizeof(TestApplication));

  if (pTestApplication)
  {
    TestApplication_Init(pTestApplication);
  }

  return pTestApplication;
}

int TestApplication_Run(TestApplication* app)
{
  // Application_Run(app);

  Window_CreateWindow((Window*)app->mainWindow, NULL);

  return Application_Run(app);
}

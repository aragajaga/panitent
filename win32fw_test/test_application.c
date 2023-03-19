#include "win32/application.h"
#include "win32/window.h"

#include "test_application.h"
#include "test_window.h"

void TestApplication_Init(TestApplication* app)
{
  Application_Init(&app->application);

  app->mainWindow = TestWindow_Create(app);
}

TestApplication* TestApplication_Create()
{
  TestApplication* app = calloc(1, sizeof(TestApplication));

  if (app)
  {
    TestApplication_Init(app);
  }

  return app;
}

int TestApplication_Run(TestApplication* app)
{
  // Application_Run(app);

  Window_CreateWindow((Window*)app->mainWindow, NULL);

  return Application_Run(app);
}

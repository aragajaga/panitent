#include "win32/application.h"
#include "win32/window.h"
#include "test_application.h"
#include "test_window.h"

void TestApplication_Init(struct TestApplication* app)
{
  Application_Init(&app->base);

  app->palette = Palette_Create();
  app->mainWindow = TestWindow_Create(app);
}

struct TestApplication* TestApplication_Create()
{
  struct TestApplication* app = calloc(1, sizeof(struct TestApplication));

  if (app)
  {
    TestApplication_Init(app);
  }

  return app;
}

int TestApplication_Run(struct TestApplication* app)
{
  // Application_Run(app);

  Window_CreateWindow((Window*)app->mainWindow, NULL);

  return Application_Run(app);
}

#include <Windows.h>

#include "test_application.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR szCmdLine, int nCmdShow)
{
  TestApplication* app = (TestApplication*)TestApplication_Create();

  return TestApplication_Run(app);
}

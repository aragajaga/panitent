#include <Windows.h>

#include "test_application.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR szCmdLine, int nCmdShow)
{
  struct TestApplication* app = TestApplication_Create();

  return TestApplication_Run(app);
}

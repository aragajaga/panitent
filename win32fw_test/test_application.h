#ifndef _TEST_APPLICATION_H
#define _TEST_APPLICATION_H

#include "win32/application.h"
#include "test_window.h"

typedef struct TestApplication TestApplication;

struct TestApplication {
  struct Application application;
  struct TestWindow* mainWindow;
};

void TestApplication_Init(TestApplication*);
TestApplication* TestApplication_Create();
int TestApplication_Run(TestApplication*);

#endif  /* _TEST_APPLICATION_H */

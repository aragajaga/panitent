#ifndef _TEST_APPLICATION_H
#define _TEST_APPLICATION_H

#include "../src/win32/application.h"
#include "test_window.h"

#include "../src/palette.h"

typedef struct TestApplication TestApplication;
struct TestApplication {
  Application base;

  TestWindow* mainWindow;
  Palette* palette;
};

void TestApplication_Init(struct TestApplication*);
struct TestApplication* TestApplication_Create();
int TestApplication_Run(struct TestApplication*);

#endif  /* _TEST_APPLICATION_H */

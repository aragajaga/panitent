#ifndef _TEST_APPLICATION_H
#define _TEST_APPLICATION_H

#include "win32/application.h"
#include "test_window.h"

#include "palette.h"

struct TestApplication {
  struct Application base;

  struct TestWindow* mainWindow;
  Palette* palette;
};

void TestApplication_Init(struct TestApplication*);
struct TestApplication* TestApplication_Create();
int TestApplication_Run(struct TestApplication*);

#endif  /* _TEST_APPLICATION_H */

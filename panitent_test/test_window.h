#ifndef _TEST_WINDOW_H_INCLUDED
#define _TEST_WINDOW_H_INCLUDED

#include "win32/window.h"
#include "palette_window.h"

typedef struct TestWindow TestWindow;

struct TestWindow {
  Window base;
  PaletteWindow* paletteWindow;
};

TestWindow* TestWindow_Create(Application*);

#endif  /* _TEST_WINDOW_H_INLUCDED */

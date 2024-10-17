#ifndef _TEST_WINDOW_H_INCLUDED
#define _TEST_WINDOW_H_INCLUDED

#include "../src/win32/window.h"
#include "../src/palette_window.h"

typedef struct TestWindow TestWindow;

struct TestWindow {
  Window base;
  PaletteWindow* paletteWindow;
};

TestWindow* TestWindow_$new(Application*);

#endif  /* _TEST_WINDOW_H_INLUCDED */

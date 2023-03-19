#ifndef _TEST_WINDOW_H_INCLUDED
#define _TEST_WINDOW_H_INCLUDED

#include "win32/window.h"
#include "win32/tree_view.h"

typedef struct TestWindow TestWindow;

struct TestWindow {
  Window base;
  TreeViewCtl treeView;
};

TestWindow* TestWindow_Create(Application*);

#endif  /* _TEST_WINDOW_H_INLUCDED */

#pragma once

#include "precomp.h"

#include "win32/window.h"

typedef struct GLWindow GLWindow;

struct GLWindow {
	Window base;
};

GLWindow* GLWindow_Create(Application*);

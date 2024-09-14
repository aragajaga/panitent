#ifndef PANITENT_PALETTE_WINDOW_H_
#define PANITENT_PALETTE_WINDOW_H_

#include "precomp.h"

#include "win32/window.h"
#include "palette.h"

typedef struct PaletteWindow PaletteWindow;

struct PaletteWindow {
    Window base;
    HBRUSH hbrChecker;
    Palette* palette;
};

PaletteWindow* PaletteWindow_Create(Palette*);

#endif  /* PANITENT_PALETTE_WINDOW_H_ */

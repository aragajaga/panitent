#ifndef PANITENT_PALETTE_H_
#define PANITENT_PALETTE_H_

#include "precomp.h"

#include <stdint.h>

typedef struct _palette_dialog {
  HWND win_handle;
} palette_dialog_t;

extern palette_dialog_t g_palette_dialog;

uint32_t abgr_to_argb(uint32_t abgr);

void register_palette_dialog(HINSTANCE hInstance);
void init_palette_window(HWND parent);

#endif /* PANITENT_PALETTE_H_ */

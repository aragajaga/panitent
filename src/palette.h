#ifndef PANITENT_PALETTE_H_
#define PANITENT_PALETTE_H_

#ifndef UNICODE
#define UNICODE
#endif  // UNICODE

#ifndef _UNICODE
#define _UNICODE
#endif  // _UNICODE

#include <windows.h>
#include <stdint.h>

extern uint32_t selected_color;

uint32_t abgr_to_argb(uint32_t abgr);

void register_palette_dialog(HINSTANCE hInstance);
void init_palette_window(HWND parent);

#endif  // PANITENT_PALETTE_H_

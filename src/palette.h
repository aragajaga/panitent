#ifndef PANITENT_PALETTE_H_
#define PANITENT_PALETTE_H_

#include "kvec.h"

typedef kvec_t(uint32_t) Palette;

Palette* Palette_Create();
size_t Palette_GetSize(Palette* palette);
uint32_t Palette_At(Palette*, size_t);
void Palette_Add(Palette* palette, uint32_t);
void Palette_InitDefault(Palette* palette);

#endif  /* PANITENT_PALETTE_H_ */

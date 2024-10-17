#ifndef PANITENT_SWATCHSTORAGE_H
#define PATENINT_SWATCHSTORAGE_H

#include "swatchstoragebase.h"

// FIXME remove
#include <stdint.h>
#include "util/vector.h"

typedef struct SwatchStorage SwatchStorage;
typedef struct SwatchStorage_vtbl SwatchStorage_vtbl;

struct SwatchStorage_vtbl {
    void (*Add)(SwatchStorage* pSwatchStorage, uint32_t color);
    void (*Clear)(SwatchStorage* pSwatchStorage);
};

struct SwatchStorage {
    SwatchStorage_vtbl* pVtbl;
    Vector colors;
};

void __impl_SwatchStorage_Add(SwatchStorage* pSwatchStorage, uint32_t color);
void __impl_SwatchStorage_Clear(SwatchStorage* pSwatchStorage);

SwatchStorage_vtbl __vtbl_SwatchStorage = {
    .Add = __impl_SwatchStorage_Add,
    .Clear = __impl_SwatchStorage_Clear
};

void SwatchStorage_Init(SwatchStorage* pSwatchStorage)
{
    pSwatchStorage->pVtbl = &__vtbl_SwatchStorage;

    Vector_Init(&pSwatchStorage->colors, sizeof(uint32_t));
}

void __impl_SwatchStorage_Add(SwatchStorage* pSwatchStorage, uint32_t color)
{
    Vector* pColors = &pSwatchStorage->colors;
    Vector_PushBack(pColors, (void*)&color);
}

void __impl_SwatchStorage_Clear(SwatchStorage* pSwatchStorage)
{
    Vector* pColors = &pSwatchStorage->colors;
}

#endif  /* PANITENT_SWATCHSTORAGE */

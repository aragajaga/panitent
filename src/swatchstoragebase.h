#ifndef PANITENT_SWATCHSTORAGEBASE_H
#define PANITENT_SWATCHSTORAGEBASE_H

typedef struct ISwatchStorage ISwatchStorage;
typedef struct ISwatchStorage_vtbl ISwatchStorage_vtbl;

struct ISwatchStorage_vtbl {
    void (*Add)(ISwatchStorage* pISwatchStorage, uint32_t color);
    void (*Clear)(ISwatchStorage* pISwatchStorage);
};

struct ISwatchStorage {
    ISwatchStorage_vtbl* pVtbl;
};

#endif  /* PANITENT_SWATCHSTORAGEBASE_H */

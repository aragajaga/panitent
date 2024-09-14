#pragma once

#include "../precomp.h"

inline void* _PntNew(size_t size, void (*pfnInitializer)(void*))
{
    void* pObject = malloc(size);
    if (pObject)
    {
        pfnInitializer(pObject);
    }

    return pObject;
}

#define PNTNEW(T) ((T*)_PntNew(sizeof(T), (void (*)(void*))T##_Init))

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct _Canvas Canvas;

typedef struct AlphaMask {
    int width;
    int height;
    size_t buffer_size;
    uint8_t* buffer;
} AlphaMask;

AlphaMask* AlphaMask_Create(int width, int height);
void AlphaMask_Delete(AlphaMask* pMask);
void AlphaMask_Clear(AlphaMask* pMask);
void AlphaMask_SetMax(AlphaMask* pMask, int x, int y, uint8_t opacity);
void AlphaMask_FillRect(AlphaMask* pMask, const RECT* pRect, uint8_t opacity);
void AlphaMask_StampCanvasAlpha(AlphaMask* pMask, int x, int y, const Canvas* pSource);

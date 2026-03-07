#include "precomp.h"

#include "alphamask.h"
#include "canvas.h"

AlphaMask* AlphaMask_Create(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return NULL;
    }

    AlphaMask* pMask = (AlphaMask*)calloc(1, sizeof(AlphaMask));
    if (!pMask)
    {
        return NULL;
    }

    pMask->width = width;
    pMask->height = height;
    pMask->buffer_size = (size_t)width * (size_t)height;
    pMask->buffer = (uint8_t*)calloc(pMask->buffer_size, sizeof(uint8_t));
    if (!pMask->buffer)
    {
        free(pMask);
        return NULL;
    }

    return pMask;
}

void AlphaMask_Delete(AlphaMask* pMask)
{
    if (!pMask)
    {
        return;
    }

    free(pMask->buffer);
    free(pMask);
}

void AlphaMask_Clear(AlphaMask* pMask)
{
    if (!pMask || !pMask->buffer)
    {
        return;
    }

    memset(pMask->buffer, 0, pMask->buffer_size);
}

void AlphaMask_SetMax(AlphaMask* pMask, int x, int y, uint8_t opacity)
{
    if (!pMask || !pMask->buffer)
    {
        return;
    }

    if (x < 0 || y < 0 || x >= pMask->width || y >= pMask->height)
    {
        return;
    }

    uint8_t* pCell = &pMask->buffer[(size_t)y * (size_t)pMask->width + (size_t)x];
    if (opacity > *pCell)
    {
        *pCell = opacity;
    }
}

void AlphaMask_FillRect(AlphaMask* pMask, const RECT* pRect, uint8_t opacity)
{
    if (!pMask || !pRect)
    {
        return;
    }

    int left = max(0, pRect->left);
    int top = max(0, pRect->top);
    int right = min(pMask->width - 1, pRect->right);
    int bottom = min(pMask->height - 1, pRect->bottom);

    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            AlphaMask_SetMax(pMask, x, y, opacity);
        }
    }
}

void AlphaMask_StampCanvasAlpha(AlphaMask* pMask, int x, int y, const Canvas* pSource)
{
    if (!pMask || !pSource || !pSource->buffer)
    {
        return;
    }

    const uint32_t* pPixels = (const uint32_t*)pSource->buffer;
    for (int sy = 0; sy < pSource->height; ++sy)
    {
        for (int sx = 0; sx < pSource->width; ++sx)
        {
            uint8_t alpha = (uint8_t)(pPixels[(size_t)sy * (size_t)pSource->width + (size_t)sx] >> 24);
            if (alpha > 0)
            {
                AlphaMask_SetMax(pMask, x + sx, y + sy, alpha);
            }
        }
    }
}

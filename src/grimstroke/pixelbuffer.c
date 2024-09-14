#include "../precomp.h"

#include "pixelbuffer.h"
#include "../util/assert.h"

void PixelBuffer_Init(PixelBuffer* pPixelBuffer, int width, int height)
{
    ASSERT(pPixelBuffer);

    memset(pPixelBuffer, 0, sizeof(PixelBuffer));

    Vector_Init(&pPixelBuffer->pBuffer, sizeof(unsigned char));
    unsigned char defaultValue = 0;
    Vector_Resize(&pPixelBuffer->pBuffer, width * height * 4, (void*)&defaultValue);

    pPixelBuffer->width = width;
    pPixelBuffer->height = height;
}

int PixelBuffer_GetWidth(PixelBuffer* pPixelBuffer)
{
    return pPixelBuffer->width;
}

int PixelBuffer_GetHeight(PixelBuffer* pPixelBuffer)
{
    return pPixelBuffer->height;
}

void PixelBuffer_Clear(PixelBuffer* pPixelBuffer, Color color)
{
    for (int i = 0; i < pPixelBuffer->width * pPixelBuffer->height * 4; i += 4)
    {
        *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, i) = color.r;
        *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, i + 1) = color.g;
        *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, i + 2) = color.b;
        *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, i + 3) = color.a;
    }
}

/* PRIVATE */
BOOL PixelBuffer_OutOfBounds(PixelBuffer* pPixelBuffer, int x, int y)
{
    return x < 0 || x >= pPixelBuffer->width || y < 0 || y >= pPixelBuffer->height;
}

void PixelBuffer_SetPixel(PixelBuffer* pPixelBuffer, int x, int y, Color color)
{
    if (PixelBuffer_OutOfBounds(pPixelBuffer, x, y))
    {
        return;
    }

    int index = (y * PixelBuffer_GetWidth(pPixelBuffer) + x) * 4;

    *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, index) = color.r;
    *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, index + 1) = color.g;
    *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, index + 2) = color.b;
    *(unsigned char*)Vector_At(&pPixelBuffer->pBuffer, index + 3) = color.a;
}

Color PixelBuffer_GetPixel(PixelBuffer* pPixelBuffer, int x, int y)
{
    ASSERT(!PixelBuffer_OutOfBounds(pPixelBuffer, x, y));

    int index = (y * PixelBuffer_GetWidth(pPixelBuffer) + x) * 4;

    Color color = { 0 };
    color.r = *(unsigned char*)Vector_At(pPixelBuffer, index);
    color.g = *(unsigned char*)Vector_At(pPixelBuffer, index + 1);
    color.b = *(unsigned char*)Vector_At(pPixelBuffer, index + 2);
    color.a = *(unsigned char*)Vector_At(pPixelBuffer, index + 3);

    return color;
}

const unsigned char* PixelBuffer_GetData(const PixelBuffer* pPixelBuffer)
{
    return (unsigned char*)Vector_At(&pPixelBuffer->pBuffer, 0);
}

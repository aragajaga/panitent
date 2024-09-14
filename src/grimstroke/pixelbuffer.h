#pragma once

#include "../util/vector.h"

typedef struct PixelBuffer PixelBuffer;

typedef struct Color
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
} Color;

struct PixelBuffer {
    int width;
    int height;
    Vector pBuffer;
};

void PixelBuffer_Init(PixelBuffer* pPixelBuffer, int width, int height);
int PixelBuffer_GetWidth(PixelBuffer* pPixelBuffer);
int PixelBuffer_GetHeight(PixelBuffer* pPixelBuffer);
void PixelBuffer_Clear(PixelBuffer* pPixelBuffer, Color color);
void PixelBuffer_SetPixel(PixelBuffer* pPixelBuffer, int x, int y, Color color);
Color PixelBuffer_GetPixel(PixelBuffer* pPixelBuffer, int x, int y);
const unsigned char* PixelBuffer_GetData(const PixelBuffer* pPixelBuffer);

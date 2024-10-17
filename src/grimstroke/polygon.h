#pragma once

#include "../util/vector.h"
#include "pixelbuffer.h"

typedef struct Point {
    int x;
    int y;
} Point;

typedef struct PolygonPath {
    Vector points;
} PolygonPath;

typedef struct PixelBuffer PixelBuffer;

PolygonPath* PolygonPath_Create();
void PolygonPath_AddPoint(PolygonPath* pPolygonPath, int x, int y);
void PolygonPath_Enclose(PolygonPath* pPolygonPath);
void Polygon_DrawOnPixelBuffer(PolygonPath* pPolygonPath, PixelBuffer* pPixelBuffer, Color color);

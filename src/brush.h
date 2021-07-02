#ifndef PANITENT_BRUSH_H_
#define PANITENT_BRUSH_H_

#include "canvas.h"

typedef struct _Brush Brush;

typedef struct _BrushBuilder {
  Brush* (*fn)(struct _BrushBuilder*, int size);
  void* userData;
} BrushBuilder;

Brush* Brush_Create(Canvas* tex);
void Brush_Draw(Brush* brush, int x, int y, Canvas* target, uint32_t color);
void Brush_DrawTo(Brush* brush, int x0, int y0, int x1, int y2, Canvas* target,
    uint32_t color);
Brush* TextureBrushBuilder(void* userData, int size);
Brush* CircleBrushBuilder(void* userData, int size);
Brush* SquareBrushBuilder(void* userData, int size);
Brush* PointBrushBuilder(void* userData, int size);
void Brush_Delete(Brush* brush);
Canvas* Brush_GetTexture(Brush* brush);

void InitializeBrushList();
BrushBuilder* Brush_GetBuilder(Brush* brush);
Brush* BrushBuilder_Build(BrushBuilder* builder, int size);
void Brush_BezierCurve(Brush* brush, Canvas* canvas, int x0, int y0, int x1,
    int y1, int x2, int y2, int x3, int y3, uint32_t color);
void Brush_BezierCurve2(Brush* brush, Canvas* canvas, int x0, int y0, int x1,
    int y1, int x2, int y2, int x3, int y3, uint32_t color);
void Brush_SetSize(Brush** brush, int size);

extern BrushBuilder g_brushList[80];
extern size_t g_brushListLen;
extern size_t g_brushSize;

extern BrushBuilder* g_pBrush;

#endif  /* PANITENT_BRUSH_H_ */

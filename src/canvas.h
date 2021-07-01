#ifndef PANITENT_CANVAS_H_
#define PANITENT_CANVAS_H_

#include "precomp.h"

#include <stdint.h>

#define CHANNEL_A_32(color) ((uint8_t)((color >> 24) & 0xFF))
#define CHANNEL_R_32(color) ((uint8_t)((color >> 16) & 0xFF))
#define CHANNEL_G_32(color) ((uint8_t)((color >> 8) & 0xFF))
#define CHANNEL_B_32(color) ((uint8_t)(color & 0xFF))

#define DROP_ALPHA_32(color) ((uint32_t)(color & 0x00FFFFFF));

#define ARGB(a, r, g, b)                                                    \
  ((uint32_t)(((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | \
              (b & 0xff)))

typedef struct _Canvas {
  int width;
  int height;
  uint8_t color_depth;
  size_t buffer_size;
  void* buffer;
} Canvas;

typedef struct _rect {
  int x0;
  int y0;
  int x1;
  int y1;
} rect_t;

void Canvas_DrawPixel(Canvas* canvas, int x, int y, uint32_t color);
void Canvas_SetPixel(Canvas* canvas, int x, int y, uint32_t color);
void Canvas_Plot(Canvas* canvas, float x, float y, float opacity);
BOOL Canvas_CheckBoundaries(Canvas* canvas, int x, int y);
uint32_t Canvas_GetPixel(Canvas* canvas, int x, int y);
void Canvas_FillSolid(Canvas* canvas, uint32_t color);
uint32_t color_opacity(uint32_t color, float opacity);
uint32_t mix(uint32_t color1, uint32_t color2);
void* Canvas_BufferAlloc(Canvas* canvas);
void Canvas_Delete(Canvas* canvas);
void Canvas_Clear(Canvas* canvas);
const void* Canvas_GetBuffer(Canvas*);
void Canvas_PasteBits(Canvas*, void*, int, int, int, int);
Canvas* Canvas_Clone(Canvas*);
Canvas* Canvas_Substitute(Canvas*, RECT* rc);
Canvas* Canvas_Create(int width, int height);
Canvas* Canvas_CreateFromBuffer(int width, int height, void* data);
void Canvas_Paste(Canvas* target, int x, int y, Canvas* source);
void Canvas_Overlay(Canvas* target, int x, int y, Canvas* source);
void Canvas_ColorStencil(Canvas* target, int x, int y, Canvas* source, uint32_t color);

#endif /* PANITENT_CANVAS_H_ */

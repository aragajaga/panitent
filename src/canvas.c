#include "precomp.h"

#include <math.h>

#include "canvas.h"
#include "viewport.h"
#include "color_context.h"

uint32_t color_opacity(uint32_t color, float opacity)
{
  return (((uint8_t)(round(opacity * 255.f))) << 24) | (color & 0x00FFFFFF);
}

uint32_t mix(uint32_t color1, uint32_t color2)
{
  float opacity = (color2 >> 24) / 255.f;

  uint8_t r =
      CHANNEL_R_32(color1) * opacity + CHANNEL_R_32(color2) * (1.f - opacity);

  uint8_t g =
      CHANNEL_G_32(color1) * opacity + CHANNEL_G_32(color2) * (1.f - opacity);

  uint8_t b =
      CHANNEL_B_32(color1) * opacity + CHANNEL_B_32(color2) * (1.f - opacity);

  /* TODO: Alpha mixing */

  return ARGB(0, r, g, b);
}

void* Canvas_BufferAlloc(Canvas* canvas)
{
  size_t buffer_size = canvas->width * canvas->height * canvas->color_depth;

  canvas->buffer_size = buffer_size;
  canvas->buffer      = calloc(buffer_size, sizeof(uint8_t));

  return canvas->buffer;
}

void Canvas_Delete(Canvas* canvas)
{
  canvas->width       = 0;
  canvas->height      = 0;
  canvas->color_depth = 0;

  canvas->buffer_size = 0;
  free(canvas->buffer);
}

BOOL Canvas_CheckBoundaries(Canvas* canvas, int x, int y)
{
  if (x >= 0 && y >= 0 && x < canvas->width && y < canvas->height) {
    return TRUE;
  }

  return FALSE;
}

void Canvas_Plot(Canvas* canvas, float x, float y, float opacity)
{
  if (!Canvas_CheckBoundaries(canvas, x, y)) {
    return;
  }

  unsigned int x_ = (unsigned int)x;
  unsigned int y_ = (unsigned int)y;

  uint32_t fg_color = g_color_context.fg_color;

  Canvas_DrawPixel(canvas, x_, y_, color_opacity(fg_color, opacity));
}

void Canvas_DrawPixel(Canvas* canvas, int x, int y, uint32_t color)
{
  if (!Canvas_CheckBoundaries(canvas, x, y)) {
    return;
  }

  unsigned int x_ = (unsigned int)x;
  unsigned int y_ = (unsigned int)y;

  uint8_t alpha = CHANNEL_A_32(color);

  if (alpha == 255) {
    Canvas_SetPixel(canvas, x_, y_, color);
  } else if (alpha == 0) {
    return;
  } else {
    uint32_t underlying   = Canvas_GetPixel(canvas, x, y);
    uint32_t result_color = mix(underlying, color);
    Canvas_SetPixel(canvas, x, y, result_color);
  }
}

uint32_t Canvas_GetPixel(Canvas* canvas, int x, int y)
{
  if (!Canvas_CheckBoundaries(canvas, x, y)) {
    return 0;
  }

  size_t pos = y * canvas->width + x;
  return ((uint32_t*)(canvas->buffer))[pos];
}

void Canvas_SetPixel(Canvas* canvas, int x, int y, uint32_t color)
{
  if (!Canvas_CheckBoundaries(canvas, x, y)) {
    return;
  }

  size_t pos                         = y * canvas->width + x;
  ((uint32_t*)(canvas->buffer))[pos] = color;
}

void Canvas_Clear(Canvas* canvas)
{
  Canvas_FillSolid(canvas, g_color_context.bg_color | 0xFF000000);
}

void Canvas_FillSolid(Canvas* canvas, uint32_t color)
{
  for (size_t i = 0; i < canvas->buffer_size / canvas->color_depth; i++) {
    ((uint32_t*)(canvas->buffer))[i] = color;
  }

  Viewport_Invalidate();
}

const void* Canvas_GetBuffer(Canvas* canvas)
{
  return canvas->buffer; 
}

void Canvas_PasteBits(Canvas* canvas, void* bits, int x, int y, int width,
    int height)
{
  char* byteCanvas = (char*)canvas->buffer;
  char* byteBufIn = (char*)bits;

  size_t startOffset = (y * canvas->width + x) * 4;
  size_t imageStride = width * 4;

  byteCanvas += startOffset;

  /* Copy stride by stride, by offseting canvas stride */
  for (int i = height; i; --i)
  {
    memcpy(byteCanvas, byteBufIn + width * i * 4, imageStride); 

    byteCanvas += canvas->width * 4;
  }

  Viewport_Invalidate();
}

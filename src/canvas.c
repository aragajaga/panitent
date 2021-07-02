#include "precomp.h"

#include <math.h>

#include "canvas.h"
#include "viewport.h"
#include "color_context.h"

float lerp(float a, float b, float f)
{
  return a + f * (b - a);
}

uint32_t color_opacity(uint32_t color, float opacity)
{
  uint8_t alpha = (uint8_t)round(opacity * (float)(color >> 24));
  return (alpha << 24) | (color & 0x00FFFFFF);
}

uint32_t mix(uint32_t base, uint32_t overlay)
{
  float baseA = (base >> 24 & 0xFF) / 255.f;
  float baseR = (base >> 16 & 0xFF) / 255.f;
  float baseG = (base >> 8  & 0xFF) / 255.f;
  float baseB = (base       & 0xFF) / 255.f;

  float overlayA = (overlay >> 24 & 0xFF) / 255.f;
  float overlayR = (overlay >> 16 & 0xFF) / 255.f;
  float overlayG = (overlay >> 8  & 0xFF) / 255.f;
  float overlayB = (overlay       & 0xFF) / 255.f;

  float blendR = baseR * overlayA + overlayR * (1.f - overlayA);
  float blendG = baseG * overlayA + overlayG * (1.f - overlayA);
  float blendB = baseB * overlayA + overlayB * (1.f - overlayA);

  float resultR = (1.f - overlayA) * baseR + overlayA * overlayR;
  float resultG = (1.f - overlayA) * baseG + overlayA * overlayG;
  float resultB = (1.f - overlayA) * baseB + overlayA * overlayB;
  float resultA = 1.f - (1.f - baseA) * (1.f - overlayA);

  uint8_t a = round(resultA * 255.f);
  uint8_t r = round(resultR * 255.f);
  uint8_t g = round(resultG * 255.f);
  uint8_t b = round(resultB * 255.f);

  return ARGB(a, r, g, b);
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

  if (alpha == 255 || alpha == 0) {
    Canvas_SetPixel(canvas, x_, y_, color);
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
  Canvas_FillSolid(canvas, g_color_context.bg_color);
}

void Canvas_FillSolid(Canvas* canvas, uint32_t color)
{
  for (size_t i = 0; i < canvas->buffer_size / canvas->color_depth; i++) {
    ((uint32_t*)(canvas->buffer))[i] = color;
  }
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

Canvas* Canvas_Clone(Canvas* canvas)
{
  Canvas* clone = calloc(1, sizeof(Canvas));
  memcpy(clone, canvas, sizeof(Canvas));

  clone->buffer = calloc(1, canvas->buffer_size);
  memcpy(clone->buffer, canvas->buffer, canvas->buffer_size);

  return clone;
}

Canvas* Canvas_Substitute(Canvas* canvas, RECT *rc)
{
  int subWidth = rc->right - rc->left;
  int subHeight = rc->bottom - rc->top;

  Canvas* sub = calloc(1, sizeof(Canvas));
  sub->width = subWidth;
  sub->height = subHeight;

  sub->buffer_size = subWidth * subHeight * 4;
  sub->buffer = calloc(1, sub->buffer_size);

  size_t origStart = (rc->top * canvas->width + rc->left) * 4;
  size_t originStride = canvas->width * 4;
  size_t destStride = subWidth * 4;

  unsigned char *pOrig = ((unsigned char*)canvas->buffer) + origStart;
  unsigned char *pDest = sub->buffer;
  for (int i = 0; i < subHeight; i++)
  {
    memcpy(pDest, pOrig, destStride);
    pDest += destStride;
    pOrig += originStride;
  }

  return sub;
}

Canvas* Canvas_Create(int width, int height)
{
  Canvas *canvas = calloc(1, sizeof(Canvas));
  canvas->width = width;
  canvas->height = height;
  canvas->color_depth = 4;
  canvas->buffer_size = width * height * 4;
  Canvas_BufferAlloc(canvas);

  return canvas;
}

Canvas* Canvas_CreateFromBuffer(int width, int height, void* data)
{
  Canvas* canvas = Canvas_Create(width, height);

  memcpy(canvas->buffer, data, canvas->buffer_size);
  return canvas;
}

void Canvas_ColorStencil(Canvas* target, int x, int y, Canvas* source,
    uint32_t color)
{
  uint32_t *pTarget = target->buffer;
  uint32_t *pSource = source->buffer;

  if (x >= target->width ||
      y >= target->height ||
      x + target->width < 0 ||
      y + target->height < 0)
    return;

  pTarget += (y < 0 ? 0 : y) * target->width + (x < 0 ? 0 : x);

  int width = source->width;
  int height = source->height;

  if (x + width > target->width)
  {
    width = target->width - x;
  }

  if (y + height > target->height)
  {
    height = target->height - y;
  }

  if (x < 0)
  {
    int x_ = abs(x);
    pSource += x_;
    width -= x_;
  }

  if (y < 0)
  {
    int y_ = abs(y);
    pSource += y_ * source->width;
    height -= y_;
  }

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      uint32_t color_ = color;

      float opacity = lerp(0, (*(pSource + j) >> 24) / 255.f,
          (color_ >> 24) / 255.f);
      uint8_t alpha = round(opacity * 255.f);

      color_ = (color & 0x00FFFFFF) | alpha << 24;

      *(pTarget + j) = mix(*(pTarget + j), color_);
    }

    pSource += source->width;
    pTarget += target->width;
  }
}

void Canvas_Overlay(Canvas* target, int x, int y, Canvas* source)
{
  uint32_t *pTarget = target->buffer;
  uint32_t *pSource = source->buffer;
  pTarget += y * target->width + x;

  for (int i = 0; i < source->height; i++) {
    for (int j = 0; j < source->width; j++) {
      *pTarget = mix(*pTarget, *pSource);
      pTarget++;
      pSource++;
    }

    pTarget += target->width - source->width;
  }
}

void Canvas_Paste(Canvas* target, int x, int y, Canvas* source)
{
  uint32_t *pTarget = target->buffer;
  uint32_t *pSource = source->buffer;
  pTarget += y * target->width + x;

  if (x >= target->width || y >= target->height)
    return;

  for (int i = 0; i < source->height; i++) {
    for (int j = 0; j < source->width; j++) {
      if (*pSource & 0xFF000000)
      {
        *pTarget = *pSource;
      }
      pTarget++;
      pSource++;
    }

    pTarget += target->width - source->width;
  }
}

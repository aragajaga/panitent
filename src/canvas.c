#include "precomp.h"

#include <math.h>

#include "canvas.h"
#include "viewport.h"
#include "color_context.h"
#include "queue.h"
#include "primitives_context.h"

#include <assert.h>

struct _Canvas
{
  int width;
  int height;
  uint8_t color_depth;
  size_t buffer_size;
  void* buffer;
};

DECLARE_TYPED_QUEUE(POINT)

void Canvas_DrawPixel(Canvas*, int, int, uint32_t);

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

Canvas* Canvas_New(int width, int height)
{
  Canvas* canvas = calloc(1, sizeof(Canvas));
  canvas->width = width;
  canvas->height = height;
  return canvas;
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

  /* TODO: Непродуманно. А если fg_color имеет альфу? */
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
}

const void* Canvas_GetBuffer(Canvas* canvas, size_t* size)
{
  *size = canvas->buffer_size;
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
}

void Canvas_FloodFill(Canvas* canvas, int x, int y, uint32_t color)
{
  assert(canvas);
  if (!canvas)
    return;

  uint32_t oldColor = Canvas_GetPixel(canvas, x, y);
  POINT nextpt = {x, y};

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
  queue_t* q = queue_new(sizeof(POINT));
  queue_push(q, &nextpt);
#else
  tqueue_t(POINT)* q = tqueue_new(POINT)();
  tqueue_push(POINT)(q, nextpt);
#endif

  while (q->length)
  {
    LPPOINT ppt = NULL;

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
    crefptr_t* ptr = queue_pop(q);
    ppt = crefptr_get(ptr);
#else
    POINT pt = tqueue_pop(POINT)(q);
    ppt = &pt;
#endif

    if (Canvas_GetPixel(canvas, ppt->x + 1, ppt->y) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x + 1, ppt->y, color);
      nextpt.x = ppt->x + 1;
      nextpt.y = ppt->y;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    if (Canvas_GetPixel(canvas, ppt->x - 1, ppt->y) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x - 1, ppt->y, color);
      nextpt.x = ppt->x - 1;
      nextpt.y = ppt->y;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    if (Canvas_GetPixel(canvas, ppt->x, ppt->y + 1) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x, ppt->y + 1, color);
      nextpt.x = ppt->x;
      nextpt.y = ppt->y + 1;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

    if (Canvas_GetPixel(canvas, ppt->x, ppt->y - 1) == oldColor)
    {
      Canvas_SetPixel(canvas, ppt->x, ppt->y - 1, color);
      nextpt.x = ppt->x;
      nextpt.y = ppt->y - 1;
#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
      queue_push(q, &nextpt);
#else
      tqueue_push(POINT)(q, nextpt);
#endif
    }

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
    crefptr_deref(ptr);
#endif
  }

#ifdef FLOODFILL_USE_VIRTUAL_QUEUE
  queue_delete(q);
#else
  tqueue_delete(POINT)(q);
#endif
}

void Canvas_DrawCircle(Canvas* canvas, int cx, int cy, int r)
{
  draw_circle(canvas, cx, cy, r);
}

void Canvas_DrawLine(Canvas* canvas, Rect rc)
{
  draw_line(canvas, rc);
}

void Canvas_DrawRectangle(Canvas* canvas, Rect rc)
{
  draw_rectangle(canvas, rc);
}

Rect Canvas_GetSize(Canvas* canvas)
{
  Rect rc = {0};
  rc.right = canvas->width;
  rc.bottom = canvas->height;
  return rc;
}

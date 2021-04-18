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

void* canvas_buffer_alloc(canvas_t* canvas)
{
  size_t buffer_size = canvas->width * canvas->height * canvas->color_depth;

  canvas->buffer_size = buffer_size;
  canvas->buffer      = calloc(buffer_size, sizeof(uint8_t));

  return canvas->buffer;
}

void canvas_delete(canvas_t* canvas)
{
  canvas->width       = 0;
  canvas->height      = 0;
  canvas->color_depth = 0;

  canvas->buffer_size = 0;
  free(canvas->buffer);
}

BOOL canvas_check_boundaries(canvas_t* canvas, int x, int y)
{
  if (x >= 0 && y >= 0 && x < canvas->width && y < canvas->height) {
    return TRUE;
  }

  return FALSE;
}

void canvas_plot(canvas_t* canvas, float x, float y, float opacity)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return;
  }

  unsigned int x_ = (unsigned int)x;
  unsigned int y_ = (unsigned int)y;

  uint32_t fg_color = g_color_context.fg_color;

  canvas_draw_pixel(canvas, x_, y_, color_opacity(fg_color, opacity));
}

void canvas_draw_pixel(canvas_t* canvas, int x, int y, uint32_t color)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return;
  }

  unsigned int x_ = (unsigned int)x;
  unsigned int y_ = (unsigned int)y;

  uint8_t alpha = CHANNEL_A_32(color);

  /* TODO: Непродуманно. А если fg_color имеет альфу? */
  if (alpha == 255) {
    canvas_set_pixel(canvas, x_, y_, color);
  } else if (alpha == 0) {
    return;
  } else {
    uint32_t underlying   = canvas_get_pixel(canvas, x, y);
    uint32_t result_color = mix(underlying, color);
    canvas_set_pixel(canvas, x, y, result_color);
  }
}

uint32_t canvas_get_pixel(canvas_t* canvas, int x, int y)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return 0;
  }

  size_t pos = y * canvas->width + x;
  return ((uint32_t*)(canvas->buffer))[pos];
}

void canvas_set_pixel(canvas_t* canvas, int x, int y, uint32_t color)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return;
  }

  size_t pos                         = y * canvas->width + x;
  ((uint32_t*)(canvas->buffer))[pos] = color;
}

void canvas_clear(canvas_t* canvas)
{
  canvas_fill_solid(canvas, g_color_context.bg_color);
}

void canvas_fill_solid(canvas_t* canvas, uint32_t color)
{
  for (size_t i = 0; i < canvas->buffer_size / canvas->color_depth; i++) {
    ((uint32_t*)(canvas->buffer))[i] = color;
  }

  viewport_invalidate();
}

const void* canvas_get_buffer(canvas_t* canvas)
{
  return canvas->buffer; 
}

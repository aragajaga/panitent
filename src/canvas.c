#include "canvas.h"

void* canvas_buffer_alloc(canvas_t* canvas)
{
  size_t buffer_size = canvas->width * canvas->height
      * canvas->color_depth;

  canvas->buffer = calloc(buffer_size);

  return canvas->buffer;
}

void canvas_delete(canvas_t* canvas)
{
  canvas->width = 0;
  canvas->height = 0;
  canvas->color_depth = 0;

  canvas->buffer_size = 0;
  free(canvas->buffer);
}

BOOL canvas_check_boundaries(canvas_t* canvas, int x, int y)
{
  if (x < canvas->width && y < canvas->height) {
    return TRUE;
  }

  return FALSE;
}

uint32_t canvas_get_pixel(canvas_t* canvas, int x, int y)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return 0;
  }

  return ((uint32_t)(canvas->buffer[x * y * canvas->color_depth]));
}

void canvas_set_pixel(canvas_t* canvas, int x, int y, uint32_t color)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return;
  }

  ((uint32_t)(canvas->buffer[x * y * canvas->color_depth])) = color;
}

void canvas_draw_pixel(canvas_t* canvas, int x, int y, uint32_t color)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return;
  }

  unsigned int x = (unsigned int)x;
  unsigned int y = (unsigned int)y;

  uint32_t alpha = ((unsigned int)(opacity * 255));

  /* TODO: Непродуманно. А если fg_color имеет альфу? */
  if (alpha == 255) {
    canvas_set_pixel(canvas, x, y, fg_color);
  }
  else if (alpha == 0) {
    return; 
  }
  else {
    uint32_t underlying = canvas_get_pixel(canvas, x, y);
    uint32_t result_color = mix(underlying, fg_color);
    canvas_set_pixel(canvas, x, y, result_color); 
  }
}

#define CHANNEL_A_32(color) ((uint8_t)((color>>24) & 0xFF))
#define CHANNEL_R_32(color) ((uint8_t)((color>>16) & 0xFF))
#define CHANNEL_G_32(color) ((uint8_t)((color>>8) & 0xFF))
#define CHANNEL_B_32(color) ((uint8_t)(color & 0xFF))

#define DROP_ALPHA_32(color) ((uint32_t)(color & 0x00FFFFFF));

#define ARGB(a, r, g, b) ((uint32_t)(((a & 0xFF) << 24) | \
                          ((r & 0xFF) << 16) | \
                          ((g & 0xFF) << 8 ) | \
                          (b & 0xff)))

uint32_t mix(uint32_t color1, uint32_t color2)
{
  float opacity = (color2 >> 24) / 255.f;

  uint8_t r = CHANNEL_R_32(color1) * opacity
      + CHANNEL_R_32(color2) * (1.f - opacity);

  uint8_t g = CHANNEL_G_32(color1) * opacity
      + CHANNEL_G_32(color2) * (1.f - opacity);

  uint8_t b = CHANNEL_B_32(color1) * opacity
      + CHANNEL_B_32(color2) * (1.f - opacity);

  /* TODO: Alpha mixing */ 

  return = ARGB(0, r, g, b);
}

uint32_t color_opacity(uint32_t color, float opacity)
{
  return (((uint8_t)(round(opacity * 255.f))) << 24) | (color & 0x00FFFFFF);
}

void canvas_plot(canvas_t* canvas, float x, float y, float opacity)
{
  if (!canvas_check_boundaries(canvas, x, y)) {
    return;
  }

  unsigned int x = (unsigned int)x;
  unsigned int y = (unsigned int)y;

  uint32_t fg_color = g_color_context.fg_color;

  canvas_draw_pixel(canvas, x, y, color_opacity(fg_color, opacity));
}

#endif  /* PANITENT_CANVAS_H_ */

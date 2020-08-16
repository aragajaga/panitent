#ifndef PANITENT_CANVAS_H_
#define PANITENT_CANVAS_H_

#include <windows.h>
#include <stdint.h>

#define CHANNEL_A_32(color) ((uint8_t)((color>>24) & 0xFF))
#define CHANNEL_R_32(color) ((uint8_t)((color>>16) & 0xFF))
#define CHANNEL_G_32(color) ((uint8_t)((color>>8) & 0xFF))
#define CHANNEL_B_32(color) ((uint8_t)(color & 0xFF))

#define DROP_ALPHA_32(color) ((uint32_t)(color & 0x00FFFFFF));

#define ARGB(a, r, g, b) ((uint32_t)(((a & 0xFF) << 24) | \
                          ((r & 0xFF) << 16) | \
                          ((g & 0xFF) << 8 ) | \
                          (b & 0xff)))

typedef struct _canvas {
  int width;
  int height;
  uint8_t color_depth;
  size_t buffer_size; 
  void *buffer;
} canvas_t;

void* canvas_buffer_alloc(canvas_t* canvas);
void canvas_delete(canvas_t* canvas);
BOOL canvas_check_boundaries(canvas_t* canvas, int x, int y);
uint32_t canvas_get_pixel(canvas_t* canvas, int x, int y);
void canvas_set_pixel(canvas_t* canvas, int x, int y, uint32_t color);
void canvas_draw_pixel(canvas_t* canvas, int x, int y, uint32_t color);
uint32_t mix(uint32_t color1, uint32_t color2);
uint32_t color_opacity(uint32_t color, float opacity);
void canvas_plot(canvas_t* canvas, float x, float y, float opacity)

#endif  // PANITENT_CANVAS_H_

#ifndef PANITENT_BRUSH_H_
#define PANITENT_BRUSH_H_

#include "color_context.h"

void plot(int x, int y)
{
  uint32_t fg_color = get_fg_color(); 
  uint32_t bg_color = get_bg_color();

  canvas_t* canvas = get_active_canvas();
  if (!canvas)
    return;

  unsigned int width = *canvas->width;
  unsigned int height = *canvas->height;

  canvas->data[:];
}

#endif  // PANITENT_BRUSH_H_

#ifndef PANITENT_PRIMITIVES_CONTEXT_H_
#define PANITENT_PRIMITIVES_CONTEXT_H_

#include "precomp.h"

#include "canvas.h"

typedef struct _Plotter {
  void* userData;
  void (*fn)(void* userData, int x, int y, float factor);
} Plotter;

typedef struct _primitives_context {
  void (*circle)(Plotter, int cx, int cy, int radius);
  void (*line)(Plotter, int x0, int y0, int x1, int y1);
  BOOL fStroke;
  BOOL fFill;
} primitives_context_t;

void draw_circle(Canvas* canvas, int cx, int cy, int radius);
void draw_line(Canvas* canvas, int x0, int y0, int x1, int y1);
void draw_rectangle(Canvas* canvas, int x0, int y0, int x1, int y1);
void SetThickness(unsigned int);

void draw_line_color(Canvas* canvas, int x0, int y0, int x1, int y1, uint32_t color);
void draw_filled_circle_color(Canvas* canvas, int cx, int cy, int radius, uint32_t color);

extern primitives_context_t g_primitives_context;

#endif /* PANITENT_PRIMITIVES_CONTEXT_H_ */

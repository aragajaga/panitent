#ifndef PANITENT_PRIMITIVES_CONTEXT_H_
#define PANITENT_PRIMITIVES_CONTEXT_H_

#include "precomp.h"

#include "canvas.h"

typedef struct _primitives_context {
  void (*circle)(Canvas* canvas, int cx, int cy, int radius);
  void (*line)(Canvas* canvas, rect_t rc);
} primitives_context_t;

void draw_circle(Canvas* canvas, int cx, int cy, int radius);
void draw_line(Canvas* canvas, rect_t rc);
void draw_rectangle(Canvas*, rect_t);
void SetThickness(unsigned int);

extern primitives_context_t g_primitives_context;

#endif /* PANITENT_PRIMITIVES_CONTEXT_H_ */

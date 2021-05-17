#ifndef PANITENT_PRIMITIVES_CONTEXT_H_
#define PANITENT_PRIMITIVES_CONTEXT_H_

#include "precomp.h"

#include "canvas.h"

typedef struct _primitives_context {
  void (*circle)(Canvas*, int, int, int);
  void (*line)(Canvas*, Rect);
} primitives_context_t;

void draw_circle(Canvas*, int, int, int);
void draw_line(Canvas*, Rect);
void draw_rectangle(Canvas*, Rect);
void SetThickness(unsigned int);

extern primitives_context_t g_primitives_context;

#endif /* PANITENT_PRIMITIVES_CONTEXT_H_ */

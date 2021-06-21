#ifndef PANITENT_WU_PRIMITIVES_H_
#define PANITENT_WU_PRIMITIVES_H_

#include "canvas.h"
#include "primitives_context.h"

#define ipart_(X)  ((int)(X))
#define round_(X)  ((int)(((double)(X)) + 0.5))
#define fpart_(X)  (((double)(X)) - (double)ipart_(X))
#define rfpart_(X) (1.0 - fpart_(X))

#define swap_(a, b)    \
  do {                 \
    __typeof__(a) tmp; \
    tmp = a;           \
    a   = b;           \
    b   = tmp;         \
  } while (0)

extern primitives_context_t g_wu_primitives;

void wu_draw_circle(Canvas* canvas, int cx, int cy, int radius);
void wu_draw_line(Canvas* canvas, rect_t rc);
void wu_init();

#endif /* PANITENT_WU_PRIMITIVES_H_ */

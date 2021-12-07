#ifndef PANITENT_WU_PRIMITIVES_H_
#define PANITENT_WU_PRIMITIVES_H_

#include "canvas.h"
#include "primitives_context.h"

#define ipart_(X)  ((int)(X))
#define round_(X)  ((int)(((float)(X)) + 0.5f))
#define fpart_(X)  (((float)(X)) - (float)ipart_(X))
#define rfpart_(X) (1.0f - fpart_(X))

#define swap_(a, b)    \
  do {                 \
    __typeof__(a) tmp; \
    tmp = a;           \
    a   = b;           \
    b   = tmp;         \
  } while (0)

#define ffswapT_(T, a, b) \
  do {                    \
    T tmp;                \
    tmp = a;              \
    a = b;                \
    b = tmp;              \
  } while (0)

extern primitives_context_t g_wu_primitives;

void wu_draw_circle(Plotter p, int cx, int cy, int radius);
void wu_draw_line(Plotter p, int x0, int y0, int x1, int y1);
void wu_init();

#endif /* PANITENT_WU_PRIMITIVES_H_ */

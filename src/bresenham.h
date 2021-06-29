#ifndef PANITENT_BRESENHAM_H_
#define PANITENT_BRESENHAM_H_

#include "precomp.h"
#include "primitives_context.h"
#include "canvas.h"

extern primitives_context_t g_bresenham_primitives;

void bresenham_circle(Plotter p, int cx, int cy, int radius);
void bresenham_filled_circle(Plotter p, int cx, int cy, int radius);
void bresenham_line(Plotter p, int x0, int y0, int x1, int y1);
inline static void bresenham_line_rc(Plotter p, rect_t rc);
void bresenham_init();

#endif /* PANITENT_BRESENHAM_H_ */

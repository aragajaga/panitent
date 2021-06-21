#ifndef PANITENT_BRESENHAM_H_
#define PANITENT_BRESENHAM_H_

#include "precomp.h"
#include "primitives_context.h"
#include "canvas.h"

extern primitives_context_t g_bresenham_primitives;

void bresenham_circle(Canvas* canvas, int cx, int cy, int radius);
void bresenham_line(Canvas* canvas, rect_t rc);
void bresenham_init();

#endif /* PANITENT_BRESENHAM_H_ */

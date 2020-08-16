#ifndef PANITENT_WU_PRIMITIVES_H_
#define PANITENT_WU_PRIMITIVES_H_

#include "canvas.h"
#include "primitives_context.h"

primitives_context_t g_wu_primitives;

void wu_draw_circle(canvas_t* canvas, int cx, int cy, int radius);
void wu_draw_line(canvas_t* canvas, RECT rc);
void wu_init();

#endif  // PANITENT_WU_PRIMITIVES_H_

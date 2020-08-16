#include "primitives_context.h"

primitives_context_t g_primitives_context;

void draw_rectangle(canvas_t* canvas, RECT rc)
{
  RECT l1 = {rc.left,  rc.top, rc.right, rc.top},
       l2 = {rc.left,  rc.top, rc.left,  rc.bottom},
       l3 = {rc.right, rc.top, rc.right, rc.bottom},
       l4 = {rc.left,  rc.top, rc.right, rc.bottom};

  draw_line(canvas, l1);
  draw_line(canvas, l2);
  draw_line(canvas, l3);
  draw_line(canvas, l4);
}

void draw_circle(canvas_t* canvas, int cx, int cy, int radius)
{
  g_primitives_context.circle(canvas, cx, cy, radius); 
}

void draw_line(canvas_t* canvas, RECT rc)
{
  g_primitives_context.line(canvas, rc); 
}

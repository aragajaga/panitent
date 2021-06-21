#include "precomp.h"

#include <stdio.h>

#include "primitives_context.h"
#include "viewport.h"

primitives_context_t g_primitives_context;
unsigned int g_thickness = 1;

void draw_rectangle(Canvas* canvas, rect_t rc)
{
  /*
   * 1. +-------->  2. +--------+
   *                            |
   *                            |
   *                            v
   *
   * 3. +--------+  4. +--------+
   *             |     ^        |
   *             |     |        |
   *    <--------+     +--------+
   */
  rect_t l1 = {rc.x0, rc.y0, rc.x1, rc.y0};
  rect_t l2 = {rc.x1, rc.y0, rc.x1, rc.y1};
  rect_t l3 = {rc.x1, rc.y1, rc.x0, rc.y1};
  rect_t l4 = {rc.x0, rc.y1, rc.x0, rc.y0};

  draw_line(canvas, l1);
  draw_line(canvas, l2);
  draw_line(canvas, l3);
  draw_line(canvas, l4);
}

void draw_circle(Canvas* canvas, int cx, int cy, int radius)
{
  g_primitives_context.circle(canvas, cx, cy, radius);
  Viewport_Invalidate();
}

void draw_line(Canvas* canvas, rect_t rc)
{
  printf("[BoundingTest] x: %d, y: %d\n", rc.x1, rc.y1);

  rc.x0 -= g_thickness / 2;
  rc.y0 -= g_thickness / 2;
  rc.x1 += g_thickness / 2;
  rc.y1 += g_thickness / 2;

  for (size_t i = 0; i < g_thickness; i++)
  {
    g_primitives_context.line(canvas, rc);
    rc.x0++;
    rc.y0++;
  }
  Viewport_Invalidate();
}

void SetThickness(unsigned int thickness)
{
  g_thickness = thickness;
}

#include "precomp.h"

#include <stdio.h>

#include "primitives_context.h"
#include "viewport.h"
#include "commontypes.h"

primitives_context_t g_primitives_context;
unsigned int g_thickness = 1;

void draw_rectangle(Canvas* canvas, Rect rc)
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
  Rect l1 = {rc.left, rc.top, rc.right, rc.top};
  Rect l2 = {rc.right, rc.top, rc.right, rc.bottom};
  Rect l3 = {rc.right, rc.bottom, rc.left, rc.bottom};
  Rect l4 = {rc.left, rc.bottom, rc.left, rc.top};

  draw_line(canvas, l1);
  draw_line(canvas, l2);
  draw_line(canvas, l3);
  draw_line(canvas, l4);
}

void draw_circle(Canvas* canvas, int cx, int cy, int radius)
{
  g_primitives_context.circle(canvas, cx, cy, radius);
}

void draw_line(Canvas* canvas, Rect rc)
{
  printf("[BoundingTest] x: %d, y: %d\n", rc.right, rc.bottom);

  rc.left -= g_thickness / 2;
  rc.top -= g_thickness / 2;
  rc.right += g_thickness / 2;
  rc.bottom += g_thickness / 2;

  for (size_t i = 0; i < g_thickness; i++)
  {
    g_primitives_context.line(canvas, rc);
    rc.left++;
    rc.top++;
  }
}

void SetThickness(unsigned int thickness)
{
  g_thickness = thickness;
}

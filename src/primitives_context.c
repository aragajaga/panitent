#include "precomp.h"

#include <stdio.h>
#include <math.h>

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
  if (g_thickness < 2)
  {
    g_primitives_context.line(canvas, rc);
    Viewport_Invalidate();
    return;
  }

  // g_primitives_context.line(canvas, rc);

  int xdif = rc.x1 - rc.x0;
  int ydif = rc.y1 - rc.y0;

  if (rc.y1 - rc.y0 != 0)
  {
    float slope = (rc.y1 - rc.y0) / (float)(rc.x1 - rc.x0);
    if (slope != 0)
    {
      float slope2 = -1.f / slope;

      float factor = g_thickness;

      if (slope2 > g_thickness)
      {
        factor /= slope2;
      }

      rect_t rc1 = {
        rc.x0 - factor,
        rc.y0 - slope2 * factor,
        rc.x0 + factor,
        rc.y0 + slope2 * factor
      };

      rect_t rc2 = {
        rc.x1 - factor,
        rc.y1 - slope2 * factor,
        rc.x1 + factor,
        rc.y1 + slope2 * factor
      };

      rect_t rc3 = {
        rc.x0 + factor,
        rc.y0 + slope2 * factor,
        rc.x1 + factor,
        rc.y1 + slope2 * factor
      };

      rect_t rc4 = {
        rc.x0 - factor,
        rc.y0 - slope2 * factor,
        rc.x1 - factor,
        rc.y1 - slope2 * factor
      };

      g_primitives_context.line(canvas, rc1);
      g_primitives_context.line(canvas, rc2);
      g_primitives_context.line(canvas, rc3);
      g_primitives_context.line(canvas, rc4);
    }
  }

  Viewport_Invalidate();
}

void SetThickness(unsigned int thickness)
{
  g_thickness = thickness;
}

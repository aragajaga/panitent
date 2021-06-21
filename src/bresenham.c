#include "bresenham.h"

primitives_context_t g_bresenham_primitives;

void bresenham_circle_plot(Canvas* canvas, int xc, int yc, int x, int y)
{
  Canvas_Plot(canvas, xc + x, yc + y, 1.f);
  Canvas_Plot(canvas, xc - x, yc + y, 1.f);
  Canvas_Plot(canvas, xc + x, yc - y, 1.f);
  Canvas_Plot(canvas, xc - x, yc - y, 1.f);
  Canvas_Plot(canvas, xc + y, yc + x, 1.f);
  Canvas_Plot(canvas, xc - y, yc + x, 1.f);
  Canvas_Plot(canvas, xc + y, yc - x, 1.f);
  Canvas_Plot(canvas, xc - y, yc - x, 1.f);
}

void bresenham_circle(Canvas* canvas, int cx, int cy, int radius)
{
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;
  bresenham_circle_plot(canvas, cx, cy, x, y);

  while (y >= x) {
    x++;

    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
    bresenham_circle_plot(canvas, cx, cy, x, y);
  }
}

int sign(int x)
{
  if (x > 0)
    return 1;
  else if (x < 0)
    return -1;
  else
    return 0;
}

void bresenham_line(Canvas* canvas, rect_t rc)
{
  int x1 = rc.x0;
  int x2 = rc.x1;
  int y1 = rc.y0;
  int y2 = rc.y1;

  int x, y, dx, dy, swap, temp, s1, s2, p, i;

  x    = x1;
  y    = y1;
  dx   = abs(x2 - x1);
  dy   = abs(y2 - y1);
  s1   = sign(x2 - x1);
  s2   = sign(y2 - y1);
  swap = 0;
  Canvas_Plot(canvas, x1, y1, 1.f);

  if (dy > dx) {
    temp = dx;
    dx   = dy;
    dy   = temp;
    swap = 1;
  }
  p = 2 * dy - dx;
  for (i = 0; i < dx; i++) {
    Canvas_Plot(canvas, x, y, 1.f);
    while (p >= 0) {
      p = p - 2 * dx;
      if (swap)
        x += s1;
      else
        y += s2;
    }
    p = p + 2 * dy;
    if (swap)
      y += s2;
    else
      x += s1;
  }
  Canvas_Plot(canvas, x, y, 1.f);
}

void bresenham_init()
{
  g_bresenham_primitives.line   = bresenham_line;
  g_bresenham_primitives.circle = bresenham_circle;
}

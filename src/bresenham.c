#include "bresenham.h"

primitives_context_t g_bresenham_primitives;

void bresenham_circle_plot(Plotter p, int xc, int yc, int x,
    int y)
{
  p.fn(p.userData, xc + x, yc + y, 1.f);
  p.fn(p.userData, xc - x, yc + y, 1.f);
  p.fn(p.userData, xc + x, yc - y, 1.f);
  p.fn(p.userData, xc - x, yc - y, 1.f);
  p.fn(p.userData, xc + y, yc + x, 1.f);
  p.fn(p.userData, xc - y, yc + x, 1.f);
  p.fn(p.userData, xc + y, yc - x, 1.f);
  p.fn(p.userData, xc - y, yc - x, 1.f);
}

static void bresenham_circle_fill(Plotter p, int xc, int yc,
    int x, int y)
{
  bresenham_line(p, xc - x, yc + y, xc + x, yc + y);
  bresenham_line(p, xc - x, yc - y, xc + x, yc - y);
  bresenham_line(p, xc - y, yc + x, xc + y, yc + x);
  bresenham_line(p, xc - y, yc - x, xc + y, yc - x);
}

void bresenham_circle(Plotter p, int cx, int cy, int radius)
{
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;
  bresenham_circle_plot(p, cx, cy, x, y);

  while (y >= x) {
    x++;

    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
    bresenham_circle_plot(p, cx, cy, x, y);
  }
}

void bresenham_filled_circle(Plotter p, int cx, int cy,
    int radius)
{
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;
  bresenham_circle_fill(p, cx, cy, x, y);

  while (y >= x) {
    x++;

    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
    bresenham_circle_fill(p, cx, cy, x, y);
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

static inline void bresenham_line_rc(Plotter p, rect_t rc)
{
  bresenham_line(p, rc.x0, rc.y0, rc.x1, rc.y1);
}

void bresenham_line(Plotter p, int x0, int y0, int x1, int y1)
{
  int x = x0;
  int y = y0;
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int s1 = sign(x1 - x0);
  int s2 = sign(y1 - y0);
  int swap = 0;
  int temp;
  int p_;
  int i;

  p.fn(p.userData, x0, y0, 1.f);

  if (dy > dx) {
    temp = dx;
    dx = dy;
    dy = temp;
    swap = 1;
  }

  p_ = 2 * dy - dx;

  for (i = 0; i < dx; i++)
  {
    p.fn(p.userData, x, y, 1.f);

    while (p_ >= 0) {
      p_ = p_ - 2 * dx;
      if (swap)
        x += s1;
      else
        y += s2;
    }
    p_ = p_ + 2 * dy;
    if (swap)
      y += s2;
    else
      x += s1;
  }

  p.fn(p.userData, x, y, 1.f);
}

void bresenham_init()
{
  g_bresenham_primitives.line   = bresenham_line;
  g_bresenham_primitives.circle = bresenham_circle;
}

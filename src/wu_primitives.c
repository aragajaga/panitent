#include "precomp.h"

#include <math.h>
#include "wu_primitives.h"

primitives_context_t g_wu_primitives;

void wu_init()
{
  g_wu_primitives.circle = wu_draw_circle;
  g_wu_primitives.line   = wu_draw_line;
}

void wu_draw_circle(Plotter p, int cx, int cy, int radius)
{

  int x   = radius;
  int y   = -1;
  float t = 0;
  while (x - 1 > y) {
    y++;

    float rp   = sqrt(radius * radius - y * y);
    float dist = ceil(rp) - rp;
    if (dist < t)
      x--;

    float alpha = 1 - dist / 2;

    
    p.fn(p.userData, cx + x, cy + y, 1);
    p.fn(p.userData, cx + x - 1, cy + y, alpha);
    p.fn(p.userData, cx + x + 1, cy + y, 0.5 - alpha);

    p.fn(p.userData, cx + y, cy + x, 1);
    p.fn(p.userData, cx + y, cy + x - 1, alpha);
    p.fn(p.userData, cx + y, cy + x + 1, 0.5 - alpha);

    p.fn(p.userData, cx - x, cy + y, 1);
    p.fn(p.userData, cx - x + 1, cy + y, alpha);
    p.fn(p.userData, cx - x - 1, cy + y, 0.5 - alpha);

    p.fn(p.userData, cx - y, cy + x, 1);
    p.fn(p.userData, cx - y, cy + x - 1, alpha);
    p.fn(p.userData, cx - y, cy + x + 1, 0.5 - alpha);

    p.fn(p.userData, cx + x, cy - y, 1);
    p.fn(p.userData, cx + x - 1, cy - y, alpha);
    p.fn(p.userData, cx + x + 1, cy - y, 0.5 - alpha);

    p.fn(p.userData, cx + y, cy - x, 1);
    p.fn(p.userData, cx + y, cy - x - 1, 0.5 - alpha);
    p.fn(p.userData, cx + y, cy - x + 1, alpha);

    p.fn(p.userData, cx - y, cy - x, 1);
    p.fn(p.userData, cx - y, cy - x - 1, 0.5 - alpha);
    p.fn(p.userData, cx - y, cy - x + 1, alpha);

    p.fn(p.userData, cx - x, cy - y, 1);
    p.fn(p.userData, cx - x - 1, cy - y, 0.5 - alpha);
    p.fn(p.userData, cx - x + 1, cy - y, alpha);

    t = dist;
  }
}

void wu_draw_line(Plotter p, int x0, int y0, int x1, int y1)
{
  float dx = (float)x1 - (float)x1;
  float dy = (float)y1 - (float)y0;

  if (fabs(dx) > fabs(dy)) {
    if (x1 < x0) {
      ffswapT_(int, x0, x1);
      ffswapT_(int, y0, y1);
    }

    float gradient = dy / dx;
    float xend     = round_(x0);
    float yend     = y0 + gradient * (xend - x0);
    float xgap     = rfpart_(x0 + 0.5);
    int xpxl1      = xend;
    int ypxl1      = ipart_(yend);
    p.fn(p.userData, xpxl1, ypxl1, 1.0 - rfpart_(yend) * xgap);
    p.fn(p.userData, xpxl1, ypxl1 + 1, 1.0 - fpart_(yend) * xgap);
    float intery = yend + gradient;

    xend      = round_(x1);
    yend      = y1 + gradient * (xend - x1);
    xgap      = fpart_(x1 + 0.5);
    int xpxl2 = xend;
    int ypxl2 = ipart_(yend);
    p.fn(p.userData, xpxl2, ypxl2, 1.f - rfpart_(yend) * xgap);
    p.fn(p.userData, xpxl2, ypxl2 + 1, 1.f - fpart_(yend) * xgap);

    int x;
    for (x = xpxl1 + 1; x < xpxl2; x++) {
      p.fn(p.userData, x, ipart_(intery), 1.f - rfpart_(intery));
      p.fn(p.userData, x, ipart_(intery) + 1, 1.f - fpart_(intery));
      intery += gradient;
    }
  } else {
    if (y1 < y0) {
      ffswapT_(int, x0, x1);
      ffswapT_(int, y0, y1);
    }
    float gradient = dx / dy;
    float yend     = round_(y0);
    float xend     = x0 + gradient * (yend - y0);
    float ygap     = rfpart_(y0 + 0.5);
    int ypxl1      = yend;
    int xpxl1      = ipart_(xend);
    p.fn(p.userData, xpxl1, ypxl1, 1.f - rfpart_(xend) * ygap);
    p.fn(p.userData, xpxl1 + 1, ypxl1, 1.f - fpart_(xend) * ygap);
    float interx = xend + gradient;

    yend      = round_(y1);
    xend      = x1 + gradient * (yend - y1);
    ygap      = fpart_(y1 + 0.5);
    int ypxl2 = yend;
    int xpxl2 = ipart_(xend);
    p.fn(p.userData, xpxl2, ypxl2, 1.f - rfpart_(xend) * ygap);
    p.fn(p.userData, xpxl2 + 1, ypxl2, 1.f - fpart_(xend) * ygap);

    int y;
    for (y = ypxl1 + 1; y < ypxl2; y++) {
      p.fn(p.userData, ipart_(interx), y, 1.f - rfpart_(interx));
      p.fn(p.userData, ipart_(interx) + 1, y, 1.f - fpart_(interx));
      interx += gradient;
    }
  }
}

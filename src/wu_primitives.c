#include "wu_primitives.h"

void wu_init()
{
  g_wu_primitives.circle = wu_draw_circle;
  g_wu_primitives.line = wu_draw_line;
}

void wu_draw_circle(canvas_t* canvas, int cx, int cy, int radius) {

  int x = radius;
  int y = -1;
  float t = 0;
  while (x - 1 > y)
  {
    y++;

    float rp = sqrt(radius*radius - y*y);
    float dist = ceil(rp) - rp;
    if (dist < t)
      x--;

    float alpha = dist/2;

    canvas_plot(canvas, cx + x,     cy + y,     1);
    canvas_plot(canvas, cx + x - 1, cy + y,     alpha);
    canvas_plot(canvas, cx + x + 1, cy + y,     0.5 - alpha);

    canvas_plot(canvas, cx + y,     cy + x,     1);
    canvas_plot(canvas, cx + y,     cy + x - 1, alpha);
    canvas_plot(canvas, cx + y,     cy + x + 1, 0.5 - alpha);

    canvas_plot(canvas, cx - x,     cy + y,     1);
    canvas_plot(canvas, cx - x + 1, cy + y,     alpha);
    canvas_plot(canvas, cx - x - 1, cy + y,     0.5 - alpha);

    canvas_plot(canvas, cx - y,     cy + x,     1);
    canvas_plot(canvas, cx - y,     cy + x - 1, alpha);
    canvas_plot(canvas, cx - y,     cy + x + 1, 0.5 - alpha);


    canvas_plot(canvas, cx + x,     cy - y,     1);
    canvas_plot(canvas, cx + x - 1, cy - y,     alpha);
    canvas_plot(canvas, cx + x + 1, cy - y,     0.5 - alpha);

    canvas_plot(canvas, cx + y,     cy - x,     1);
    canvas_plot(canvas, cx + y,     cy - x - 1, 0.5 - alpha);
    canvas_plot(canvas, cx + y,     cy - x + 1, alpha);

    canvas_plot(canvas, cx - y,     cy - x,     1);
    canvas_plot(canvas, cx - y,     cy - x - 1, 0.5 - alpha);
    canvas_plot(canvas, cx - y,     cy - x + 1, alpha);

    canvas_plot(canvas, cx - x,     cy - y,     1);
    canvas_plot(canvas, cx - x - 1, cy - y,     0.5 - alpha);
    canvas_plot(canvas, cx - x + 1, cy - y,     alpha);

    t = dist;
  }
}

void wu_draw_line(canvas_t* canvas, RECT rc) {

  unsigned int x1 = rc.left;
  unsigned int y1 = rc.top;
  unsigned int x2 = rc.right;
  unsigned int y2 = rc.bottom;

  float dx = (float)x2 - (float)x1;
  float dy = (float)y2 - (float)y1;

  if ( fabs(dx) > fabs(dy) )
  {
    if ( x2 < x1 )
    {
      swap_(x1, x2);
      swap_(y1, y2);
    }

    float gradient = dy / dx;
    float xend = round_(x1);
    float yend = y1 + gradient*(xend - x1);
    float xgap = rfpart_(x1 + 0.5);
    int xpxl1 = xend;
    int ypxl1 = ipart_(yend);
    canvas_plot(canvas, xpxl1, ypxl1, rfpart_(yend)*xgap);
    canvas_plot(canvas, xpxl1, ypxl1+1, fpart_(yend)*xgap);
    float intery = yend + gradient;

    xend = round_(x2);
    yend = y2 + gradient*(xend - x2);
    xgap = fpart_(x2+0.5);
    int xpxl2 = xend;
    int ypxl2 = ipart_(yend);
    canvas_plot(canvas, xpxl2, ypxl2, rfpart_(yend) * xgap);
    canvas_plot(canvas, xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

    int x;
    for(x=xpxl1+1; x < xpxl2; x++)
    {
      canvas_plot(canvas, x, ipart_(intery), rfpart_(intery));
      canvas_plot(canvas, x, ipart_(intery) + 1, fpart_(intery));
      intery += gradient;
    }
  } else {
    if ( y2 < y1 )
    {
      swap_(x1, x2);
      swap_(y1, y2);
    }
    float gradient = dx / dy;
    float yend = round_(y1);
    float xend = x1 + gradient*(yend - y1);
    float ygap = rfpart_(y1 + 0.5);
    int ypxl1 = yend;
    int xpxl1 = ipart_(xend);
    canvas_plot(canvas, xpxl1, ypxl1, rfpart_(xend)*ygap);
    canvas_plot(canvas, xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
    float interx = xend + gradient;

    yend = round_(y2);
    xend = x2 + gradient*(yend - y2);
    ygap = fpart_(y2+0.5);
    int ypxl2 = yend;
    int xpxl2 = ipart_(xend);
    canvas_plot(canvas, xpxl2, ypxl2, rfpart_(xend) * ygap);
    canvas_plot(canvas, xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

    int y;
    for(y=ypxl1+1; y < ypxl2; y++)
    {
      canvas_plot(canvas, ipart_(interx), y, rfpart_(interx));
      canvas_plot(canvas, ipart_(interx) + 1, y, fpart_(interx));
      interx += gradient;
    }
  }
}

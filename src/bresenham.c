#include "bresenham.h"

primitives_context_t g_bresenham_primitives;

void bresenham_circle_plot(canvas_t* canvas, int xc, int yc, int x, int y)
{
  canvas_plot(canvas, xc+x, yc+y, 1.f);
  canvas_plot(canvas, xc-x, yc+y, 1.f);
  canvas_plot(canvas, xc+x, yc-y, 1.f);
  canvas_plot(canvas, xc-x, yc-y, 1.f);
  canvas_plot(canvas, xc+y, yc+x, 1.f);
  canvas_plot(canvas, xc-y, yc+x, 1.f);
  canvas_plot(canvas, xc+y, yc-x, 1.f);
  canvas_plot(canvas, xc-y, yc-x, 1.f);
}

void bresenham_circle(canvas_t* canvas, int cx, int cy, int radius)
{
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;
  bresenham_circle_plot(canvas, cx, cy, x, y);

  while (y >= x)
  {
    x++;

    if (d > 0)
    {
      y--;
      d = d + 4 * (x - y) + 10;
    }
    else {
      d = d + 4 * x + 6;
    }
    bresenham_circle_plot(canvas, cx, cy, x, y);
  }
}

int sign(int x)
{
  if(x>0)
    return 1;
  else if(x<0)
    return -1;
  else
    return 0;
}

void bresenham_line(canvas_t* canvas, RECT rc)
{
  int x1 = rc.left;
  int x2 = rc.right;
  int y1 = rc.top;
  int y2 = rc.bottom;

  int x,y,dx,dy,swap,temp,s1,s2,p,i;

  x=x1;
  y=y1;
  dx=abs(x2-x1);
  dy=abs(y2-y1);
  s1=sign(x2-x1);
  s2=sign(y2-y1);
  swap=0;
  canvas_plot(canvas, x1, y1, 1.f);

  if(dy>dx)
  {
    temp=dx;
    dx=dy;
    dy=temp;
    swap=1;
  }
  p=2*dy-dx;
  for(i=0;i<dx;i++)
  {
    canvas_plot(canvas, x, y, 1.f);
    while(p>=0)
    {
      p=p-2*dx;
      if(swap)
        x+=s1;
      else
        y+=s2;
    }
    p=p+2*dy;
    if(swap)
      y+=s2;
    else
      x+=s1;
  }
  canvas_plot(canvas, x, y, 1.f);
}

void bresenham_init()
{
  g_bresenham_primitives.line = bresenham_line;
  g_bresenham_primitives.circle = bresenham_circle;
}

#include "precomp.h"

#include <stdio.h>
#include <math.h>

#include "bresenham.h"
#include "primitives_context.h"
#include "color_context.h"
#include "viewport.h"

typedef struct _Point {
  int x;
  int y;
} Point;

typedef struct _PlotterData {
  Canvas* canvas;
  uint32_t color;
} PlotterData;

typedef struct _PolygonPath {
  Point points[80];
  size_t count;
} PolygonPath;

primitives_context_t g_primitives_context;
unsigned int g_thickness = 1;

PolygonPath* PolygonPath_Create();
void PolygonPath_AddPoint(PolygonPath* poly, int x, int y);
void PolygonPath_Enclose(PolygonPath* poly);
void PolygonPath_Fill(Canvas* canvas, PolygonPath* poly);

void PixelPlotterCallback(void* userData, int x, int y, float factor)
{
  PlotterData *data = (PlotterData*) userData;
  Canvas_DrawPixel(data->canvas, x, y, color_opacity(data->color, factor));
}

void draw_rectangle(Canvas* canvas, int x0, int y0, int x1, int y1)
{
  if (g_primitives_context.fFill)
  {
    Plotter p;
    PlotterData *pdat = calloc(1, sizeof(PlotterData));
    pdat->canvas = canvas;
    pdat->color = g_color_context.bg_color;
    p.userData = pdat;
    p.fn = PixelPlotterCallback;

    for (int i = 0; i < y1 - y0; i++)
    {
      g_primitives_context.line(p, x0, y0 + i, x1, y0 + i);
    }

    free(p.userData);

    Viewport_Invalidate();
  }

  if (g_primitives_context.fStroke)
  {
    draw_line(canvas, x0, y0, x1, y0);
    draw_line(canvas, x1, y0, x1, y1);
    draw_line(canvas, x1, y1, x0, y1);
    draw_line(canvas, x0, y1, x0, y0);
  }
}

void draw_circle(Canvas* canvas, int cx, int cy, int radius)
{
  if (g_primitives_context.fFill)
  {
    Plotter p;
    PlotterData *pdat = calloc(1, sizeof(PlotterData));
    pdat->canvas = canvas;
    pdat->color = g_color_context.bg_color;
    p.userData = pdat;
    p.fn = PixelPlotterCallback;

    bresenham_filled_circle(p, cx, cy, radius);

    free(p.userData);
  }

  if (g_primitives_context.fStroke)
  {
    Plotter p;
    PlotterData *pdat = calloc(1, sizeof(PlotterData));
    pdat->canvas = canvas;
    pdat->color = g_color_context.fg_color;
    p.userData = pdat;
    p.fn = PixelPlotterCallback;

    g_primitives_context.circle(p, cx, cy, radius);

    free(p.userData);
  }
  Viewport_Invalidate();
}

void draw_filled_circle_color(Canvas* canvas, int cx, int cy, int radius, uint32_t color)
{
  Plotter p;
  PlotterData *pdat = calloc(1, sizeof(PlotterData));
  pdat->canvas = canvas;
  pdat->color = color;
  p.userData = pdat;
  p.fn = PixelPlotterCallback;

  bresenham_filled_circle(p, cx, cy, radius);

  free(p.userData);
}

void draw_line_color(Canvas* canvas, int x0, int y0, int x1, int y1,
    uint32_t color)
{
  if (g_thickness < 2)
  {
    Plotter p;
    PlotterData *pdat = calloc(1, sizeof(PlotterData));
    pdat->canvas = canvas;
    pdat->color = g_color_context.fg_color;
    p.userData = pdat;
    p.fn = PixelPlotterCallback;

    g_primitives_context.line(p, x0, y0, x1, y1);

    free(p.userData);

    Viewport_Invalidate();
    return;
  }

  int dx = x1 - x0;
  int dy = y1 - y0;
  float slope;

  if (dx == 0)
    slope = 0.0f;

  if (dy == 0)
    slope = 1.0f;

  if ((dx != 0) && (dy != 0))
  {
    slope = (y1 - y0) / (float)(x1 - x0);
  }

  if (slope != 0)
  {
    float slope2 = -1.f / slope;

    float d = g_thickness / sqrt(pow(slope2, 2) + 1.f) / 2;

    PolygonPath *poly = PolygonPath_Create();
    PolygonPath_AddPoint(poly, x0 - d, y0 - slope2 * d);
    PolygonPath_AddPoint(poly, x1 - d, y1 - slope2 * d);
    PolygonPath_AddPoint(poly, x1 + d, y1 + slope2 * d);
    PolygonPath_AddPoint(poly, x0 + d, y0 + slope2 * d);
    PolygonPath_Enclose(poly);
    PolygonPath_Fill(canvas, poly);
    free(poly);

    Plotter p;
    PlotterData *pdat = calloc(1, sizeof(PlotterData));
    pdat->canvas = canvas;
    pdat->color = color;
    p.userData = pdat;
    p.fn = PixelPlotterCallback;

    g_primitives_context.line(p, x0 - d, y0 - slope2 * d, x0 + d, y0 + slope2 * d);
    g_primitives_context.line(p, x1 - d, y1 - slope2 * d, x1 + d, y1 + slope2 * d);
    g_primitives_context.line(p, x0 + d, y0 + slope2 * d, x1 + d, y1 + slope2 * d);
    g_primitives_context.line(p, x0 - d, y0 - slope2 * d, x1 - d, y1 - slope2 * d);
    free(p.userData);

  }

  Viewport_Invalidate();
}

void draw_line(Canvas* canvas, int x0, int y0, int x1, int y1)
{
  draw_line_color(canvas, x0, y0, x1, y1, g_color_context.fg_color);
}

void SetThickness(unsigned int thickness)
{
  g_thickness = thickness;
}

PolygonPath* PolygonPath_Create()
{
  PolygonPath* poly = calloc(1, sizeof(PolygonPath));
  return poly;
}

void PolygonPath_AddPoint(PolygonPath* poly, int x, int y)
{
  Point p = {x, y};
  poly->points[poly->count++] = p;
}

void PolygonPath_Enclose(PolygonPath* poly)
{
  poly->points[poly->count] = poly->points[0];
}

void PolygonPath_Fill(Canvas* canvas, PolygonPath* poly)
{
  Plotter p;
  PlotterData *pdat = calloc(1, sizeof(PlotterData));
  pdat->canvas = canvas;
  pdat->color = g_color_context.fg_color;
  p.userData = pdat;
  p.fn = PixelPlotterCallback;

  float slope[80] = {0};
  Point *points = poly->points;

  for (size_t i = 0; i < poly->count; i++)
  {
    int dx = points[i + 1].x - points[i].x;
    int dy = points[i + 1].y - points[i].y;

    if (dx == 0)
      slope[i] = 0.0f;

    if (dy == 0)
      slope[i] = 1.0f;

    if ((dx != 0) && (dy != 0))
    {
      slope[i] = dx/(float)dy;
    }
  }

  for (int y = 0; y < canvas->height; y++)
  {
    int xi[80] = {0};
    int k = 0;

    for (size_t i = 0; i < poly->count; i++)
    {
      if (((points[i].y <= y) && (points[i + 1].y >  y)) ||
          ((points[i].y >  y) && (points[i + 1].y <= y)))
      {
        xi[k] = (int)(points[i].x + slope[i] * (y - points[i].y));
        k++;
      }
    }

    for (int j = 0; j < k - 1; j++) {
      for (int i = 0; i < k - 1; i++) {
        if (xi[i] > xi[i + 1]) {
          int temp = xi[i];
          xi[i] = xi[i + 1];
          xi[i + 1] = temp;
        }
      }
    }

    for (int i = 0; i < k; i+=2)
    {
      g_primitives_context.line(p, xi[i], y, xi[i + 1], y);
    }
  }
free(p.userData);
}

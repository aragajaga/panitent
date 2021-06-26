#include "precomp.h"

#include <stdio.h>
#include <math.h>

#include "primitives_context.h"
#include "viewport.h"

typedef struct _Point {
  int x;
  int y;
} Point;

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

  int dx = rc.x1 - rc.x0;
  int dy = rc.y1 - rc.y0;
  float slope;

  if (dx == 0)
    slope = 0.0f;

  if (dy == 0)
    slope = 1.0f;

  if ((dx != 0) && (dy != 0))
  {
    slope = (rc.y1 - rc.y0) / (float)(rc.x1 - rc.x0);
  }

  if (slope != 0)
  {
    float slope2 = -1.f / slope;

    float d = g_thickness / sqrt(pow(slope2, 2) + 1.f) / 2;

    rect_t rc1 = {
      rc.x0 - d,
      rc.y0 - slope2 * d,
      rc.x0 + d,
      rc.y0 + slope2 * d
    };

    rect_t rc2 = {
      rc.x1 - d,
      rc.y1 - slope2 * d,
      rc.x1 + d,
      rc.y1 + slope2 * d
    };

    rect_t rc3 = {
      rc.x0 + d,
      rc.y0 + slope2 * d,
      rc.x1 + d,
      rc.y1 + slope2 * d
    };

    rect_t rc4 = {
      rc.x0 - d,
      rc.y0 - slope2 * d,
      rc.x1 - d,
      rc.y1 - slope2 * d
    };

    g_primitives_context.line(canvas, rc1);
    g_primitives_context.line(canvas, rc2);
    g_primitives_context.line(canvas, rc3);
    g_primitives_context.line(canvas, rc4);

    PolygonPath *poly = PolygonPath_Create();
    PolygonPath_AddPoint(poly, rc.x0 - d, rc.y0 - slope2 * d);
    PolygonPath_AddPoint(poly, rc.x1 - d, rc.y1 - slope2 * d);
    PolygonPath_AddPoint(poly, rc.x1 + d, rc.y1 + slope2 * d);
    PolygonPath_AddPoint(poly, rc.x0 + d, rc.y0 + slope2 * d);
    PolygonPath_Enclose(poly);
    PolygonPath_Fill(canvas, poly);

    free(poly);
  }

  Viewport_Invalidate();
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
      rect_t rc = { xi[i], y, xi[i + 1], y };
      g_primitives_context.line(canvas, rc);
    }
  }
}

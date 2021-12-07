#include "precomp.h"

#include "brush.h"
#include "canvas.h"
#include <assert.h>
#include <math.h>
#include "primitives_context.h"

typedef struct _Brush {
  Canvas* tex;
  float distance;
  BrushBuilder* builder;
} Brush;

BrushBuilder g_brushList[80];
BrushBuilder* g_pBrush;
size_t g_brushListLen;
int g_brushSize;

Brush* PointBrushBuilder_Build(BrushBuilder* builder, int size);
Brush* CircleBrushBuilder_Build(BrushBuilder* builder, int size);
Brush* SquareBrushBuilder_Build(BrushBuilder* builder, int size);
Brush* TextureBrushBuilder_Build(BrushBuilder* builder, int size);

BrushBuilder GetPointBrushBuilder()
{
  BrushBuilder builder = {0};
  builder.fn = PointBrushBuilder_Build;
  return builder;
}

BrushBuilder GetCircleBrushBuilder()
{
  BrushBuilder builder = {0};
  builder.fn = CircleBrushBuilder_Build;
  return builder;
}

BrushBuilder GetSquareBrushBuilder()
{
  BrushBuilder builder = {0};
  builder.fn = SquareBrushBuilder_Build;
  return builder;
}

BrushBuilder GetTextureBrushBuilder(Canvas* tex)
{
  BrushBuilder builder = {0};
  builder.fn = TextureBrushBuilder_Build;
  builder.userData = tex;
  return builder;
}

Brush* BrushBuilder_Build(BrushBuilder* builder, int size)
{
  return (*builder->fn)(builder, size);
}

void InitializeBrushList()
{
  g_brushList[g_brushListLen++] = GetPointBrushBuilder();
  g_brushList[g_brushListLen++] = GetCircleBrushBuilder();
  g_brushList[g_brushListLen++] = GetSquareBrushBuilder();
}


BrushBuilder* Brush_GetBuilder(Brush* brush)
{
  return brush->builder;
}

void Brush_SetBuilder(Brush* brush, BrushBuilder* builder)
{
  brush->builder = builder;
}

float euclidean_distance(float px, float py)
{
  return 1.f - min(sqrtf(powf(px, 2.f) + powf(py, 2.f)), 1.f);
}

Brush* Brush_Create(Canvas* tex)
{
  assert(tex);
  if (!tex)
    return NULL;

  Brush *brush = calloc(1, sizeof(Brush));
  if (brush)
  {
    brush->distance = 0.25;
    brush->tex = tex;
  }

  return brush;
}

Brush* TextureBrushBuilder_Build(BrushBuilder* builder, int size)
{
  UNREFERENCED_PARAMETER(size);

  Canvas* tex = (Canvas*)builder->userData;
  Brush* brush = Brush_Create(tex);
  Brush_SetBuilder(brush, builder);
  return brush;
}

Brush* PointBrushBuilder_Build(BrushBuilder* builder, int size)
{
  Canvas* tex = Canvas_Create(size, size);
  uint32_t *buffer = tex->buffer;
  for (int y = 0; y < tex->height; y++)
  {
    for (int x = 0; x < tex->width; x++)
    {
      float value = euclidean_distance(
          (x / (float)tex->width - 0.5f) * 2,
          (y / (float)tex->height - 0.5f) * 2) * 255.f;

      uint8_t byte = (uint8_t)(round(value));

      buffer[x + tex->width * y] = byte << 24;
    }
  }

  Brush* brush = Brush_Create(tex);
  Brush_SetBuilder(brush, builder);
  return brush;
}

Brush* CircleBrushBuilder_Build(BrushBuilder* builder, int size)
{
  Canvas* tex = Canvas_Create(size, size);
  draw_filled_circle_color(tex, size/2, size/2, size/2, 0xFF000000);

  Brush* brush = Brush_Create(tex);
  Brush_SetBuilder(brush, builder);
  return brush;
}

Brush* SquareBrushBuilder_Build(BrushBuilder* builder, int size)
{
  Canvas* tex = Canvas_Create(size, size);
  Canvas_FillSolid(tex, 0xFF000000);

  Brush* brush = Brush_Create(tex);
  Brush_SetBuilder(brush, builder);
  return brush;
}

void Brush_Draw(Brush* brush, int x, int y, Canvas* target, uint32_t color)
{
  Canvas_ColorStencil(target, x - brush->tex->width/2, y - brush->tex->height/2,
      brush->tex, color);
}

static int sign(int x)
{
  if (x > 0)
    return 1;
  else if (x < 0)
    return -1;
  else
    return 0;
}

void Brush_DrawTo(Brush* brush, int x0, int y0, int x1, int y1, Canvas* target,
    uint32_t color)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int signx = sign(x1 - x0);
  int signy = sign(y1 - y0);

  if (dy > dx)
  {
    float slope = dx/(float)dy;

    for (int i = 0; i < dy; i++)
    {
      Brush_Draw(brush, x0 + (int)roundf((float)i * slope * (float)signx), y0 + i * signy, target, color);
    }
  }
  else {
    float slope = dy/(float)dx;

    for (int i = 0; i < dx; i++)
    {
      Brush_Draw(brush, x0 + i * signx, y0 + (int)roundf((float)i * slope * (float)signy), target, color);
    }
  }
}

void Brush_Delete(Brush* brush)
{
  assert(brush);
  if (!brush)
    return;

  Canvas_Delete(brush->tex);
  free(brush);
}

Canvas* Brush_GetTexture(Brush* brush)
{
  assert(brush);
  if (!brush)
    return NULL;

  return brush->tex;
}

inline static int BezierGetPt(int n1, int n2, float perc)
{
  int diff = n2 - n1;
  return n1 + (int)((float)diff * perc);
}

void Brush_BezierCurve(Brush* brush, Canvas* canvas,
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    int x3, int y3,
    uint32_t color)
{
  for (float i = 0; i < 1.f; i += 0.01f)
  {
    int xa = BezierGetPt(x0, x1, i);
    int ya = BezierGetPt(y0, y1, i);
    int xb = BezierGetPt(x1, x2, i);
    int yb = BezierGetPt(y1, y2, i);
    int xc = BezierGetPt(x2, x3, i);
    int yc = BezierGetPt(y2, y3, i);

    int xm = BezierGetPt(xa, xb, i);
    int ym = BezierGetPt(ya, yb, i);
    int xn = BezierGetPt(xb, xc, i);
    int yn = BezierGetPt(yb, yc, i);

    int x = BezierGetPt(xm, xn, i);
    int y = BezierGetPt(ym, yn, i);

    Brush_Draw(brush, x, y, canvas, color);
  }
}

void Brush_BezierCurve2(Brush* brush, Canvas* canvas,
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    int x3, int y3,
    uint32_t color)
{
  for (float u = 0.f; u <= 1.f; u += 0.01f)
  {
    float xu =
      powf(1.f - u, 3.f) * (float)x0 + 3.f * u *
      powf(1.f - u, 2.f) * (float)x1 + 3.f *
      powf(u, 2.f) * (1.f - u) * (float)x2 +
      powf(u, 3.f) * (float)x3;

    float yu =
      powf(1.f - u, 3.f) * (float)y0 + 3.f * u *
      powf(1.f - u, 2.f) * (float)y1 + 3.f *
      powf(u, 2.f) * (1.f - u) * (float)y2 +
      powf(u, 3.f) * (float)y3;

    Brush_Draw(brush, (int)xu, (int)yu, canvas, color);
  }
}

void Brush_SetSize(Brush** brush, int size)
{
  BrushBuilder* builder = Brush_GetBuilder(*brush);
  assert(builder);
  if (!builder)
    return;

  Brush* newBrush = BrushBuilder_Build(builder, size);
  Brush_Delete(*brush);

  *brush = newBrush;
}

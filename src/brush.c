#include "precomp.h"

#include "brush.h"
#include "canvas.h"
#include <assert.h>
#include <math.h>

typedef struct _Brush {
  Canvas* tex;
  float distance;
} Brush;

float euclidean_distance(float px, float py)
{
  return 1.f - min(sqrt(pow(px, 2) + pow(py, 2)), 1.f);
}

Brush* Brush_Create(Canvas* tex)
{
  assert(tex);
  if (!tex)
    return NULL;

  Brush *brush = calloc(1, sizeof(Brush));
  brush->distance = 0.25;

  Canvas* canvas = Canvas_Create(16, 16);
  uint32_t *buffer = canvas->buffer;
  for (int y = 0; y < canvas->height; y++)
  {
    for (int x = 0; x < canvas->width; x++)
    {
      float value = euclidean_distance(
          (x / (float)canvas->width - 0.5f) * 2,
          (y / (float)canvas->height - 0.5f) * 2) * 255.f;
      
      uint8_t byte = (uint8_t)(round(value));

      buffer[x + canvas->width * y] = byte << 24;
    }
  }
  brush->tex = canvas;

  return brush;
}

void Brush_Draw(Brush* brush, int x, int y, Canvas* target)
{
  Canvas_ColorStencil(target, x - brush->tex->width/2, y - brush->tex->height/2, brush->tex);
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

void Brush_DrawTo(Brush* brush, int x0, int y0, int x1, int y1, Canvas* target)
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
      Brush_Draw(brush, x0 + i * slope * signx, y0 + i * signy, target); 
    }
  }
  else {
    float slope = dy/(float)dx;

    for (int i = 0; i < dx; i++)
    {
      Brush_Draw(brush, x0 + i * signx, y0 + i * slope * signy, target); 
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

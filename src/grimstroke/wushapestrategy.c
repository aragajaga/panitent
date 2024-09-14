#include "../precomp.h"

#include <math.h>

#include "wushapestrategy.h"
#include "shapecontext.h"
#include "plotter.h"

#include "../util/assert.h"

#define ipart_(X)  ((int)(X))
#define round_(X)  ((int)(((float)(X)) + 0.5f))
#define fpart_(X)  (((float)(X)) - (float)ipart_(X))
#define rfpart_(X) (1.0f - fpart_(X))

#define ffswapT_(T, a, b) \
  do {                    \
    T tmp;                \
    tmp = a;              \
    a = b;                \
    b = tmp;              \
  } while (0)

void WuShapeStrategy_Init(WuShapeStrategy* pWuShapeStrategy);
void WuShapeStrategy_DrawLine(WuShapeStrategy* pWuShapeStrategy, int x1, int y1, int x2, int y2);
void WuShapeStrategy_DrawCircle(WuShapeStrategy* pWuShapeStrategy, int x, int y, int radius);

WuShapeStrategy* WuShapeStrategy_Create()
{
    WuShapeStrategy* pWuShapeStrategy = (WuShapeStrategy*)malloc(sizeof(WuShapeStrategy));

    if (pWuShapeStrategy)
    {
        WuShapeStrategy_Init(pWuShapeStrategy);
    }

    return pWuShapeStrategy;
}

void WuShapeStrategy_Init(WuShapeStrategy* pWuShapeStrategy)
{
    ASSERT(pWuShapeStrategy);
    ShapeStrategy_Init(&pWuShapeStrategy->base);

    pWuShapeStrategy->base.DrawLine = WuShapeStrategy_DrawLine;
    pWuShapeStrategy->base.DrawCircle = WuShapeStrategy_DrawCircle;
}

void WuShapeStrategy_DrawLine(WuShapeStrategy* pWuShapeStrategy, int x1, int y1, int x2, int y2)
{
    ASSERT(pWuShapeStrategy && pWuShapeStrategy->base.m_pShapeContext);
    Plotter* pPlotter = ShapeContext_GetPlotter(pWuShapeStrategy->base.m_pShapeContext);

    int strokeWidth = 32;

    BOOL steep = fabsf((float)(y2 - y1)) > fabsf((float)(x2 - x1)) != 0.0f;

    if (steep) {
        ffswapT_(int, x1, y1);
        ffswapT_(int, x2, y2);
    }

    if (x1 > x2) {
        ffswapT_(int, x1, x2);
        ffswapT_(int, y1, y2);
    }

    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    float gradient = dy / dx;

    // Handle the stroke width by iterating through each offset
    for (int i = -strokeWidth / 2; i <= strokeWidth / 2; i++) {
        float offset = (float)i;

        // Handle first endpoint
        float xend = round_((float)x1);
        float yend = (float)y1 + gradient * (xend - (float)x1) + offset;
        float xgap = rfpart_((float)x1 + 0.5f);
        int xpxl1 = (int)xend;
        int ypxl1 = ipart_(yend);

        if (steep) {
            pPlotter->fn(pPlotter->userData, ypxl1, xpxl1, (unsigned char)(rfpart_(yend) * xgap * 255.0f));
            pPlotter->fn(pPlotter->userData, ypxl1 + 1, xpxl1, (unsigned char)(fpart_(yend) * xgap * 255.0f));
        }
        else {
            pPlotter->fn(pPlotter->userData, xpxl1, ypxl1, (unsigned char)(rfpart_(yend) * xgap * 255.0f));
            pPlotter->fn(pPlotter->userData, xpxl1, ypxl1 + 1, (unsigned char)(fpart_(yend) * xgap * 255.0f));
        }
        float intery = yend + gradient;

        // Handle second endpoint
        xend = round_((float)x2);
        yend = (float)y2 + gradient * (xend - (float)x2) + offset;
        xgap = fpart_((float)x2 + 0.5f);
        int xpxl2 = (int)xend;
        int ypxl2 = ipart_(yend);

        if (steep) {
            pPlotter->fn(pPlotter->userData, ypxl2, xpxl2, (unsigned char)(rfpart_(yend) * xgap * 255.0f));
            pPlotter->fn(pPlotter->userData, ypxl2 + 1, xpxl2, (unsigned char)(fpart_(yend) * xgap * 255.0f));
        }
        else {
            pPlotter->fn(pPlotter->userData, xpxl2, ypxl2, (unsigned char)(rfpart_(yend) * xgap * 255.0f));
            pPlotter->fn(pPlotter->userData, xpxl2, ypxl2 + 1, (unsigned char)(fpart_(yend) * xgap * 255.0f));
        }

        // Plot the line
        if (steep) {
            for (int x = xpxl1 + 1; x < xpxl2; x++) {
                pPlotter->fn(pPlotter->userData, ipart_(intery), x, (unsigned char)(rfpart_(intery) * 255.0f));
                pPlotter->fn(pPlotter->userData, ipart_(intery) + 1, x, (unsigned char)(fpart_(intery) * 255.0f));
                intery += gradient;
            }
        }
        else {
            for (int x = xpxl1 + 1; x < xpxl2; x++) {
                pPlotter->fn(pPlotter->userData, x, ipart_(intery), (unsigned char)(rfpart_(intery) * 255.0f));
                pPlotter->fn(pPlotter->userData, x, ipart_(intery) + 1, (unsigned char)(fpart_(intery) * 255.0f));
                intery += gradient;
            }
        }
    }
}

void WuShapeStrategy_DrawCircle(WuShapeStrategy* pWuShapeStrategy, int x, int y, int radius)
{
    ASSERT(pWuShapeStrategy && pWuShapeStrategy->base.m_pShapeContext);
    Plotter* pPlotter = ShapeContext_GetPlotter(pWuShapeStrategy->base.m_pShapeContext);

    int dx = radius;
    int dy = -1;
    float t = 0;

    while (dx - 1 > dy) {
        dy++;

        float rp = sqrtf(powf((float)radius, 2.f) - powf((float)dy, 2.f));
        float dist = ceilf(rp) - rp;
        if (dist < t)
        {
            dx--;
        }

        unsigned char alpha = (1.f - dist / 2.f) * 255.0f;
        unsigned char halfAlpha = 0x7F - alpha;

        pPlotter->fn(pPlotter->userData, x + dx, y + dy, 0xFF);
        pPlotter->fn(pPlotter->userData, x + dx - 1, y + dy, alpha);
        pPlotter->fn(pPlotter->userData, x + dx + 1, y + dy, halfAlpha);

        pPlotter->fn(pPlotter->userData, x + dy, y + dx, 1);
        pPlotter->fn(pPlotter->userData, x + dy, y + dx - 1, alpha);
        pPlotter->fn(pPlotter->userData, x + dy, y + dx + 1, halfAlpha);

        pPlotter->fn(pPlotter->userData, x - dx, y + dy, 1);
        pPlotter->fn(pPlotter->userData, x - dx + 1, y + dy, alpha);
        pPlotter->fn(pPlotter->userData, x - dx - 1, y + dy, halfAlpha);

        pPlotter->fn(pPlotter->userData, x - dy, y + dx, 1);
        pPlotter->fn(pPlotter->userData, x - dy, y + dx - 1, alpha);
        pPlotter->fn(pPlotter->userData, x - dy, y + dx + 1, halfAlpha);

        pPlotter->fn(pPlotter->userData, x + dx, y - dy, 1);
        pPlotter->fn(pPlotter->userData, x + dx - 1, y - dy, alpha);
        pPlotter->fn(pPlotter->userData, x + dx + 1, y - dy, halfAlpha);

        pPlotter->fn(pPlotter->userData, x + dy, y - dx, 1);
        pPlotter->fn(pPlotter->userData, x + dy, y - dx - 1, halfAlpha);
        pPlotter->fn(pPlotter->userData, x + dy, y - dx + 1, alpha);

        pPlotter->fn(pPlotter->userData, x - dy, y - dx, 1);
        pPlotter->fn(pPlotter->userData, x - dy, y - dx - 1, halfAlpha);
        pPlotter->fn(pPlotter->userData, x - dy, y - dx + 1, alpha);

        pPlotter->fn(pPlotter->userData, x - dx, y - dy, 1);
        pPlotter->fn(pPlotter->userData, x - dx - 1, y - dy, halfAlpha);
        pPlotter->fn(pPlotter->userData, x - dx + 1, y - dy, alpha);

        t = dist;
    }
}

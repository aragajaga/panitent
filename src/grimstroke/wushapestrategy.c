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
static void WuShapeStrategy_AccumulateCircleSymmetry(BYTE* pCoverage, int extent, int x, int y, unsigned char opacity);
static void WuShapeStrategy_AccumulatePoint(BYTE* pCoverage, int extent, int x, int y, unsigned char opacity);

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

    int strokeWidth = 1;

    BOOL steep = fabsf((float)(y2 - y1)) > fabsf((float)(x2 - x1));

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
    if (dx == 0.0f)
    {
        pPlotter->fn(pPlotter->userData, x1, y1, 0xFF);
        return;
    }
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

    if (radius <= 0)
    {
        pPlotter->fn(pPlotter->userData, x, y, 0xFF);
        return;
    }

    int extent = radius + 2;
    int size = extent * 2 + 1;
    BYTE* pCoverage = (BYTE*)calloc((size_t)size * (size_t)size, sizeof(BYTE));
    if (!pCoverage)
    {
        return;
    }

    float radiusSq = (float)(radius * radius);
    for (int dy = 0;; ++dy)
    {
        float ySq = (float)(dy * dy);
        if (ySq > radiusSq)
        {
            break;
        }

        float xReal = sqrtf(radiusSq - ySq);
        int xBase = (int)floorf(xReal);
        if (xBase < dy)
        {
            break;
        }

        float frac = fpart_(xReal);
        unsigned char alphaBase = (unsigned char)roundf(rfpart_(xReal) * 255.0f);
        unsigned char alphaOuter = (unsigned char)roundf(frac * 255.0f);

        if (alphaBase > 0)
        {
            WuShapeStrategy_AccumulateCircleSymmetry(pCoverage, extent, xBase, dy, alphaBase);
        }
        if (alphaOuter > 0)
        {
            WuShapeStrategy_AccumulateCircleSymmetry(pCoverage, extent, xBase + 1, dy, alphaOuter);
        }
    }

    for (int dx = 0;; ++dx)
    {
        float xSq = (float)(dx * dx);
        if (xSq > radiusSq)
        {
            break;
        }

        float yReal = sqrtf(radiusSq - xSq);
        int yBase = (int)floorf(yReal);
        if (yBase < dx)
        {
            break;
        }

        float frac = fpart_(yReal);
        unsigned char alphaBase = (unsigned char)roundf(rfpart_(yReal) * 255.0f);
        unsigned char alphaOuter = (unsigned char)roundf(frac * 255.0f);

        if (alphaBase > 0)
        {
            WuShapeStrategy_AccumulateCircleSymmetry(pCoverage, extent, dx, yBase, alphaBase);
        }
        if (alphaOuter > 0)
        {
            WuShapeStrategy_AccumulateCircleSymmetry(pCoverage, extent, dx, yBase + 1, alphaOuter);
        }
    }

    for (int oy = -extent; oy <= extent; ++oy)
    {
        for (int ox = -extent; ox <= extent; ++ox)
        {
            BYTE opacity = pCoverage[(size_t)(oy + extent) * (size_t)size + (size_t)(ox + extent)];
            if (opacity > 0)
            {
                pPlotter->fn(pPlotter->userData, x + ox, y + oy, opacity);
            }
        }
    }

    free(pCoverage);
}

static void WuShapeStrategy_AccumulateCircleSymmetry(BYTE* pCoverage, int extent, int x, int y, unsigned char opacity)
{
    POINT pts[8] = {
        { x, y },
        { -x, y },
        { x, -y },
        { -x, -y },
        { y, x },
        { -y, x },
        { y, -x },
        { -y, -x }
    };

    for (int i = 0; i < ARRAYSIZE(pts); ++i)
    {
        BOOL duplicate = FALSE;
        for (int j = 0; j < i; ++j)
        {
            if (pts[i].x == pts[j].x && pts[i].y == pts[j].y)
            {
                duplicate = TRUE;
                break;
            }
        }

        if (!duplicate)
        {
            WuShapeStrategy_AccumulatePoint(pCoverage, extent, pts[i].x, pts[i].y, opacity);
        }
    }
}

static void WuShapeStrategy_AccumulatePoint(BYTE* pCoverage, int extent, int x, int y, unsigned char opacity)
{
    if (!pCoverage)
    {
        return;
    }

    int size = extent * 2 + 1;
    int ix = x + extent;
    int iy = y + extent;
    if (ix < 0 || iy < 0 || ix >= size || iy >= size)
    {
        return;
    }

    BYTE* pCell = &pCoverage[(size_t)iy * (size_t)size + (size_t)ix];
    if (opacity > *pCell)
    {
        *pCell = opacity;
    }
}

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
static float WuShapeStrategy_Clamp01(float value);
static float WuShapeStrategy_SignedDistanceToBox(float px, float py, float hx, float hy);
static void WuShapeStrategy_DrawLineBox(Plotter* pPlotter, float x1, float y1, float x2, float y2, int strokeWidth);
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
    ShapeContext* pShapeContext = pWuShapeStrategy->base.m_pShapeContext;
    Plotter* pPlotter = ShapeContext_GetPlotter(pShapeContext);
    int strokeWidth = ShapeContext_GetStrokeWidth(pShapeContext);

    WuShapeStrategy_DrawLineBox(
        pPlotter,
        (float)x1 + 0.5f,
        (float)y1 + 0.5f,
        (float)x2 + 0.5f,
        (float)y2 + 0.5f,
        strokeWidth);
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

static float WuShapeStrategy_Clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }

    return value;
}

static float WuShapeStrategy_SignedDistanceToBox(float px, float py, float hx, float hy)
{
    float dx = fabsf(px) - hx;
    float dy = fabsf(py) - hy;
    float outsideX = max(dx, 0.0f);
    float outsideY = max(dy, 0.0f);
    float outside = sqrtf(outsideX * outsideX + outsideY * outsideY);
    float inside = min(max(dx, dy), 0.0f);
    return outside + inside;
}

static void WuShapeStrategy_DrawLineBox(Plotter* pPlotter, float x1, float y1, float x2, float y2, int strokeWidth)
{
    if (!pPlotter)
    {
        return;
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    float halfThickness = max(1, strokeWidth) * 0.5f;

    if (length <= 0.0001f)
    {
        int minX = (int)floorf(x1 - halfThickness - 1.0f);
        int maxX = (int)ceilf(x1 + halfThickness + 1.0f);
        int minY = (int)floorf(y1 - halfThickness - 1.0f);
        int maxY = (int)ceilf(y1 + halfThickness + 1.0f);

        for (int py = minY; py <= maxY; ++py)
        {
            for (int px = minX; px <= maxX; ++px)
            {
                float localX = ((float)px + 0.5f) - x1;
                float localY = ((float)py + 0.5f) - y1;
                float distance = WuShapeStrategy_SignedDistanceToBox(localX, localY, halfThickness, halfThickness);
                float coverage = WuShapeStrategy_Clamp01(0.5f - distance);
                if (coverage > 0.0f)
                {
                    Plotter_PlotExact(pPlotter, px, py, (unsigned char)roundf(coverage * 255.0f));
                }
            }
        }
        return;
    }

    float dirX = dx / length;
    float dirY = dy / length;
    float normalX = -dirY;
    float normalY = dirX;
    float centerX = (x1 + x2) * 0.5f;
    float centerY = (y1 + y2) * 0.5f;
    float halfLength = length * 0.5f + 0.5f;

    float extentX = fabsf(dirX) * halfLength + fabsf(normalX) * halfThickness;
    float extentY = fabsf(dirY) * halfLength + fabsf(normalY) * halfThickness;

    int minX = (int)floorf(centerX - extentX - 1.0f);
    int maxX = (int)ceilf(centerX + extentX + 1.0f);
    int minY = (int)floorf(centerY - extentY - 1.0f);
    int maxY = (int)ceilf(centerY + extentY + 1.0f);

    for (int py = minY; py <= maxY; ++py)
    {
        for (int px = minX; px <= maxX; ++px)
        {
            float sampleX = ((float)px + 0.5f) - centerX;
            float sampleY = ((float)py + 0.5f) - centerY;
            float localX = sampleX * dirX + sampleY * dirY;
            float localY = sampleX * normalX + sampleY * normalY;
            float distance = WuShapeStrategy_SignedDistanceToBox(localX, localY, halfLength, halfThickness);
            float coverage = WuShapeStrategy_Clamp01(0.5f - distance);

            if (coverage > 0.0f)
            {
                Plotter_PlotExact(pPlotter, px, py, (unsigned char)roundf(coverage * 255.0f));
            }
        }
    }
}

#include "../precomp.h"

#include "bresenhamshapestrategy.h"
#include "plotter.h"

#include "../util/assert.h"
#include "../util.h"

void BresenhamShapeStrategy_Init(BresenhamShapeStrategy* pBresenhamShapeStrategy);
void BresenhamShapeStrategy_DrawLine(BresenhamShapeStrategy* pBresenhamShapeStrategy, int x1, int y1, int x2, int y2);
void BresenhamShapeStrategy_DrawCircle(BresenhamShapeStrategy* pBresenhamShapeStrategy, int x, int y, int radius);

BresenhamShapeStrategy* BresenhamShapeStrategy_Create()
{
    BresenhamShapeStrategy* pBresenhamShapeStrategy = (BresenhamShapeStrategy*)malloc(sizeof(BresenhamShapeStrategy));

    if (pBresenhamShapeStrategy)
    {
        BresenhamShapeStrategy_Init(pBresenhamShapeStrategy);
    }

    return pBresenhamShapeStrategy;
}

void BresenhamShapeStrategy_Init(BresenhamShapeStrategy* pBresenhamShapeStrategy)
{
    ASSERT(pBresenhamShapeStrategy);
    ShapeStrategy_Init(&pBresenhamShapeStrategy->base);

    pBresenhamShapeStrategy->base.DrawLine = BresenhamShapeStrategy_DrawLine;
    pBresenhamShapeStrategy->base.DrawCircle = BresenhamShapeStrategy_DrawCircle;
}

void BresenhamShapeStrategy_DrawLine(BresenhamShapeStrategy* pBresenhamShapeStrategy, int x1, int y1, int x2, int y2)
{
    ASSERT(pBresenhamShapeStrategy && pBresenhamShapeStrategy->base.m_pShapeContext);
    Plotter* pPlotter = ShapeContext_GetPlotter(pBresenhamShapeStrategy->base.m_pShapeContext);

    int x = x1;
    int y = y1;
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int s1 = sign(x2 - x1);
    int s2 = sign(y2 - y1);
    int swap = 0;
    int temp;
    int p_;
    int i;

    pPlotter->fn(pPlotter->userData, x1, y1, 0xFF);

    if (dy > dx) {
        temp = dx;
        dx = dy;
        dy = temp;
        swap = 1;
    }

    p_ = 2 * dy - dx;

    for (i = 0; i < dx; i++)
    {
        pPlotter->fn(pPlotter->userData, x, y, 0xFF);

        while (p_ >= 0) {
            p_ = p_ - 2 * dx;
            if (swap)
                x += s1;
            else
                y += s2;
        }
        p_ = p_ + 2 * dy;
        if (swap)
            y += s2;
        else
            x += s1;
    }

    pPlotter->fn(pPlotter->userData, x, y, 0xFF);
}

/*
[PRIVATE]
*/
void BresenhamShapeStrategy_CirclePlot(BresenhamShapeStrategy* pBresenhamShapeStrategy, Plotter* pPlotter, int xc, int yc, int x, int y)
{
    pPlotter->fn(pPlotter->userData, xc + x, yc + y, 0xFF);
    pPlotter->fn(pPlotter->userData, xc - x, yc + y, 0xFF);
    pPlotter->fn(pPlotter->userData, xc + x, yc - y, 0xFF);
    pPlotter->fn(pPlotter->userData, xc - x, yc - y, 0xFF);
    pPlotter->fn(pPlotter->userData, xc + y, yc + x, 0xFF);
    pPlotter->fn(pPlotter->userData, xc - y, yc + x, 0xFF);
    pPlotter->fn(pPlotter->userData, xc + y, yc - x, 0xFF);
    pPlotter->fn(pPlotter->userData, xc - y, yc - x, 0xFF);
}

void BresenhamShapeStrategy_DrawCircle(BresenhamShapeStrategy* pBresenhamShapeStrategy, int x, int y, int radius)
{
    ASSERT(pBresenhamShapeStrategy && pBresenhamShapeStrategy->base.m_pShapeContext);
    Plotter* pPlotter = ShapeContext_GetPlotter(pBresenhamShapeStrategy->base.m_pShapeContext);

    int dx = 0;
    int dy = radius;
    int d = 3 - 2 * radius;
    BresenhamShapeStrategy_CirclePlot(pBresenhamShapeStrategy, pPlotter, x, y, dx, dy);

    while (dy >= dx) {
        dx++;

        if (d > 0) {
            dy--;
            d = d + 4 * (dx - dy) + 10;
        }
        else {
            d = d + 4 * dx + 6;
        }
        BresenhamShapeStrategy_CirclePlot(pBresenhamShapeStrategy, pPlotter, x, y, dx, dy);
    }
}

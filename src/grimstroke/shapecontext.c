#include "../precomp.h"

#include "shapecontext.h"
#include "shapestrategy.h"

#include "../util/assert.h"

void ShapeContext_Init(ShapeContext* pShapeContext);

ShapeContext* ShapeContext_Create()
{
    ShapeContext* pShapeContext = (ShapeContext*)malloc(sizeof(ShapeContext));

    if (pShapeContext)
    {
        ShapeContext_Init(pShapeContext);
    }

    return pShapeContext;
}

void ShapeContext_Init(ShapeContext* pShapeContext)
{
    ASSERT(pShapeContext);
    memset(pShapeContext, 0, sizeof(ShapeContext));
}

void ShapeContext_DrawLine(ShapeContext* pShapeContext, int x1, int y1, int x2, int y2)
{
    /* The subject is NULL */
    ASSERT(pShapeContext);
    /* Shape strategy not set */
    ASSERT(pShapeContext->m_pShapeStrategy);
    /* Plotter not set */
    ASSERT(pShapeContext->m_pPlotter);
    pShapeContext->m_pShapeStrategy->DrawLine(pShapeContext->m_pShapeStrategy, x1, y1, x2, y2);
}

void ShapeContext_DrawCircle(ShapeContext* pShapeContext, int x, int y, int radius)
{
    ASSERT(pShapeContext && pShapeContext->m_pShapeStrategy && pShapeContext->m_pPlotter);
    pShapeContext->m_pShapeStrategy->DrawCircle(pShapeContext->m_pShapeStrategy, x, y, radius);
}


void ShapeContext_SetStrategy(ShapeContext* pShapeContext, ShapeStrategy* pShapeStrategy)
{
    ASSERT(pShapeContext && pShapeStrategy);
    ShapeStrategy_SetContext(pShapeStrategy, pShapeContext);
    pShapeContext->m_pShapeStrategy = pShapeStrategy;
}

ShapeStrategy* ShapeContext_GetStrategy(ShapeContext* pShapeContext)
{
    ASSERT(pShapeContext);
    return pShapeContext->m_pShapeStrategy;
}

void ShapeContext_SetPlotter(ShapeContext* pShapeContext, Plotter* pPlotter)
{
    ASSERT(pShapeContext);
    pShapeContext->m_pPlotter = pPlotter;
}

Plotter* ShapeContext_GetPlotter(ShapeContext* pShapeContext)
{
    ASSERT(pShapeContext);
    return pShapeContext->m_pPlotter;
}

BOOL ShapeContext_IsFillEnabled(ShapeContext* pShapeContext)
{
    return pShapeContext->m_fFill;
}

BOOL ShapeContext_IsStrokeEnabled(ShapeContext* pShapeContext)
{
    return pShapeContext->m_fStroke;
}

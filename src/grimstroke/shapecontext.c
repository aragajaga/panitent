#include "../precomp.h"

#include "shapecontext.h"
#include "shapestrategy.h"
#include "plotter.h"

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
    pShapeContext->m_fStroke = TRUE;
    pShapeContext->m_fFill = FALSE;
    pShapeContext->m_nStrokeWidth = 1;
    pShapeContext->m_pOwnedPlotter = (Plotter*)calloc(1, sizeof(Plotter));
    pShapeContext->m_pOwnedPlotterData = (PlotterData*)calloc(1, sizeof(PlotterData));
    if (pShapeContext->m_pOwnedPlotter && pShapeContext->m_pOwnedPlotterData)
    {
        pShapeContext->m_pOwnedPlotter->fn = PixelPlotterCallback;
        pShapeContext->m_pOwnedPlotter->userData = pShapeContext->m_pOwnedPlotterData;
        pShapeContext->m_pOwnedPlotterData->thickness = 1;
    }
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
    ASSERT(pShapeContext);
    return pShapeContext->m_fFill;
}

BOOL ShapeContext_IsStrokeEnabled(ShapeContext* pShapeContext)
{
    ASSERT(pShapeContext);
    return pShapeContext->m_fStroke;
}

void ShapeContext_SetFillEnabled(ShapeContext* pShapeContext, BOOL fFill)
{
    ASSERT(pShapeContext);
    pShapeContext->m_fFill = fFill ? TRUE : FALSE;
}

void ShapeContext_SetStrokeEnabled(ShapeContext* pShapeContext, BOOL fStroke)
{
    ASSERT(pShapeContext);
    pShapeContext->m_fStroke = fStroke ? TRUE : FALSE;
}

void ShapeContext_SetStrokeWidth(ShapeContext* pShapeContext, int nStrokeWidth)
{
    ASSERT(pShapeContext);
    pShapeContext->m_nStrokeWidth = max(1, nStrokeWidth);
    if (pShapeContext->m_pOwnedPlotterData)
    {
        pShapeContext->m_pOwnedPlotterData->thickness = pShapeContext->m_nStrokeWidth;
    }
}

int ShapeContext_GetStrokeWidth(ShapeContext* pShapeContext)
{
    ASSERT(pShapeContext);
    return max(1, pShapeContext->m_nStrokeWidth);
}

BOOL ShapeContext_BeginDraw(ShapeContext* pShapeContext, Canvas* pCanvas, uint32_t color)
{
    ASSERT(pShapeContext);
    if (!pShapeContext || !pCanvas || !pShapeContext->m_pOwnedPlotter || !pShapeContext->m_pOwnedPlotterData)
    {
        return FALSE;
    }

    pShapeContext->m_pOwnedPlotterData->canvas = pCanvas;
    pShapeContext->m_pOwnedPlotterData->mask = NULL;
    pShapeContext->m_pOwnedPlotterData->color = color;
    pShapeContext->m_pOwnedPlotterData->thickness = ShapeContext_GetStrokeWidth(pShapeContext);
    pShapeContext->m_pOwnedPlotter->fn = PixelPlotterCallback;
    pShapeContext->m_pPlotter = pShapeContext->m_pOwnedPlotter;
    return TRUE;
}

BOOL ShapeContext_BeginMaskDraw(ShapeContext* pShapeContext, AlphaMask* pMask)
{
    ASSERT(pShapeContext);
    if (!pShapeContext || !pMask || !pShapeContext->m_pOwnedPlotter || !pShapeContext->m_pOwnedPlotterData)
    {
        return FALSE;
    }

    pShapeContext->m_pOwnedPlotterData->canvas = NULL;
    pShapeContext->m_pOwnedPlotterData->mask = pMask;
    pShapeContext->m_pOwnedPlotterData->thickness = ShapeContext_GetStrokeWidth(pShapeContext);
    pShapeContext->m_pOwnedPlotter->fn = MaskPlotterCallback;
    pShapeContext->m_pPlotter = pShapeContext->m_pOwnedPlotter;
    return TRUE;
}

void ShapeContext_SetDrawColor(ShapeContext* pShapeContext, uint32_t color)
{
    ASSERT(pShapeContext);
    if (!pShapeContext || !pShapeContext->m_pOwnedPlotterData)
    {
        return;
    }

    pShapeContext->m_pOwnedPlotterData->color = color;
}

void ShapeContext_EndDraw(ShapeContext* pShapeContext)
{
    ASSERT(pShapeContext);
    if (!pShapeContext)
    {
        return;
    }

    if (pShapeContext->m_pPlotter == pShapeContext->m_pOwnedPlotter)
    {
        pShapeContext->m_pPlotter = NULL;
    }
    if (pShapeContext->m_pOwnedPlotterData)
    {
        pShapeContext->m_pOwnedPlotterData->canvas = NULL;
        pShapeContext->m_pOwnedPlotterData->mask = NULL;
    }
}

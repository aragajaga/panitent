#pragma once

#include <stdint.h>

typedef struct ShapeStrategy ShapeStrategy;
typedef struct Plotter Plotter;
typedef struct PlotterData PlotterData;
typedef struct _Canvas Canvas;

typedef struct ShapeContext {
    ShapeStrategy* m_pShapeStrategy;
    Plotter* m_pPlotter;
    Plotter* m_pOwnedPlotter;
    PlotterData* m_pOwnedPlotterData;
    BOOL m_fFill;
    BOOL m_fStroke;
    int m_nStrokeWidth;
} ShapeContext;

ShapeContext* ShapeContext_Create();
void ShapeContext_DrawLine(ShapeContext* pShapeContext, int x1, int y1, int x2, int y2);
void ShapeContext_DrawCircle(ShapeContext* pShapeContext, int x, int y, int radius);
void ShapeContext_SetStrategy(ShapeContext* pShapeContext, ShapeStrategy* pShapeStrategy);
ShapeStrategy* ShapeContext_GetStrategy(ShapeContext* pShapeContext);
void ShapeContext_SetPlotter(ShapeContext* pShapeContext, Plotter* pPlotter);
Plotter* ShapeContext_GetPlotter(ShapeContext* pShapeContext);
BOOL ShapeContext_IsFillEnabled(ShapeContext* pShapeContext);
BOOL ShapeContext_IsStrokeEnabled(ShapeContext* pShapeContext);
void ShapeContext_SetFillEnabled(ShapeContext* pShapeContext, BOOL fFill);
void ShapeContext_SetStrokeEnabled(ShapeContext* pShapeContext, BOOL fStroke);
void ShapeContext_SetStrokeWidth(ShapeContext* pShapeContext, int nStrokeWidth);
int ShapeContext_GetStrokeWidth(ShapeContext* pShapeContext);
BOOL ShapeContext_BeginDraw(ShapeContext* pShapeContext, Canvas* pCanvas, uint32_t color);
void ShapeContext_SetDrawColor(ShapeContext* pShapeContext, uint32_t color);
void ShapeContext_EndDraw(ShapeContext* pShapeContext);

#pragma once

typedef struct ShapeStrategy ShapeStrategy;
typedef struct Plotter Plotter;

typedef struct ShapeContext {
    ShapeStrategy* m_pShapeStrategy;
    Plotter* m_pPlotter;
    BOOL m_fFill;
    BOOL m_fStroke;
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

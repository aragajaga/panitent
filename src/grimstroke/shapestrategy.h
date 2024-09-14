#pragma once

typedef struct ShapeContext ShapeContext;
typedef struct ShapeStrategy ShapeStrategy;
typedef struct ShapeStrategy {
    ShapeContext* m_pShapeContext;
    void (*DrawCircle)(ShapeStrategy* pShapeStrategy, int x, int y, int radius);
    void (*DrawLine)(ShapeStrategy* pShapeStrategy, int x1, int y1, int x2, int y2);
} ShapeStrategy;

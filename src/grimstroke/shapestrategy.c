#include "../precomp.h"

#include "shapestrategy.h"

#include "../util/assert.h"

void ShapeStrategy_Init(ShapeStrategy* pShapeStrategy)
{
    ASSERT(pShapeStrategy);
    memset(pShapeStrategy, 0, sizeof(ShapeStrategy));
}

void ShapeStrategy_SetContext(ShapeStrategy* pShapeStrategy, ShapeContext* pShapeContext)
{
    ASSERT(pShapeStrategy && pShapeContext);
    pShapeStrategy->m_pShapeContext = pShapeContext;
}

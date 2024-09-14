#pragma once

#include "shapestrategy.h"

typedef struct WuShapeStrategy {
    ShapeStrategy base;
} WuShapeStrategy;

WuShapeStrategy* WuShapeStrategy_Create();

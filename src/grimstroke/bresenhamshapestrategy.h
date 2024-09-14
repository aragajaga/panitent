#pragma once

#include "shapestrategy.h"

typedef struct BresenhamShapeStrategy {
    ShapeStrategy base;
} BresenhamShapeStrategy;

BresenhamShapeStrategy* BresenhamShapeStrategy_Create();

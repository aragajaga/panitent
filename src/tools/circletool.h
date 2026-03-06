#pragma once

#include <stdint.h>

#include "../tool.h"

typedef struct CircleTool CircleTool;
struct CircleTool {
    Tool base;

    BOOL fDraw;
    POINT circCenter;
    uint32_t strokeColor;
    uint32_t fillColor;
};

CircleTool* CircleTool_Create();
void CircleTool_Init(CircleTool* pCircleTool);

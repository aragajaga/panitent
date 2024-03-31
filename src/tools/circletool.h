#pragma once

#include "../tool.h"

typedef struct CircleTool CircleTool;
struct CircleTool {
    Tool base;

    BOOL fDraw;
    POINT circCenter;
};

CircleTool* CircleTool_Create();
void CircleTool_Init(CircleTool* pCircleTool);

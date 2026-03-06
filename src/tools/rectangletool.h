#pragma once

#include <stdint.h>

#include "../tool.h"

typedef struct RectangleTool RectangleTool;
struct RectangleTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
    uint32_t strokeColor;
    uint32_t fillColor;
};

RectangleTool* RectangleTool_Create();
void RectangleTool_Init(RectangleTool* pRectangleTool);

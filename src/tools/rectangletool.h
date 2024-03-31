#pragma once

#include "../tool.h"

typedef struct RectangleTool RectangleTool;
struct RectangleTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
};

RectangleTool* RectangleTool_Create();
void RectangleTool_Init(RectangleTool* pRectangleTool);

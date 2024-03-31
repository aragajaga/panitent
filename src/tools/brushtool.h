#pragma once

#include "../tool.h"

typedef struct BrushTool BrushTool;
struct BrushTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
};

BrushTool* BrushTool_Create();
void BrushTool_Init(BrushTool* pBrushTool);

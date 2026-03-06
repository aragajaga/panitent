#pragma once

#include <stdint.h>

#include "../tool.h"

typedef struct BrushTool BrushTool;
struct BrushTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
    uint32_t drawColor;
};

BrushTool* BrushTool_Create();
void BrushTool_Init(BrushTool* pBrushTool);

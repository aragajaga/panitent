#pragma once

#include <stdint.h>

#include "../tool.h"
#include "../alphamask.h"
#include "../canvas.h"

typedef struct BrushTool BrushTool;
struct BrushTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
    uint32_t drawColor;
    Canvas* pStrokeBase;
    AlphaMask* pStrokeMask;
};

BrushTool* BrushTool_Create();
void BrushTool_Init(BrushTool* pBrushTool);

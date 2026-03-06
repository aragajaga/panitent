#pragma once

#include <stdint.h>

#include "../tool.h"

typedef struct LineTool LineTool;
struct LineTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
    uint32_t drawColor;
};

LineTool* LineTool_Create();
void LineTool_Init(LineTool* pLineTool);

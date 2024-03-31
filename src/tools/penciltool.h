#pragma once

#include "../tool.h"

typedef struct PencilTool PencilTool;
struct PencilTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
};

PencilTool* PencilTool_Create();
void PencilTool_Init(PencilTool* pPencilTool);

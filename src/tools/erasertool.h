#pragma once

#include "../tool.h"

typedef struct EraserTool EraserTool;
struct EraserTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
};

EraserTool* EraserTool_Create();
void EraserTool_Init(EraserTool* pEraserTool);

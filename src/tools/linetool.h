#pragma once

#include "../tool.h"

typedef struct LineTool LineTool;
struct LineTool {
    Tool base;

    BOOL fDraw;
    POINT prev;
};

LineTool* LineTool_Create();
void LineTool_Init(LineTool* pLineTool);

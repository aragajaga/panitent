#pragma once

#include "../tool.h"

typedef struct FillTool FillTool;
struct FillTool {
    Tool base;
};

FillTool* FillTool_Create();
void FillTool_Init(FillTool* pFillTool);

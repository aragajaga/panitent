#pragma once

#include "../tool.h"

typedef struct PointerTool PointerTool;
struct PointerTool {
    Tool base;
};

PointerTool* PointerTool_Create();
void PointerTool_Init(PointerTool* pPointerTool);

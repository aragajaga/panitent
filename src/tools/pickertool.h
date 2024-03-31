#pragma once

#include "../tool.h"

typedef struct PickerTool PickerTool;
struct PickerTool {
    Tool base;
};

PickerTool* PickerTool_Create();
void PickerTool_Init(PickerTool* pPickerTool);

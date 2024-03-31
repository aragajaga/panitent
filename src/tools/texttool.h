#pragma once

#include "../tool.h"

typedef struct TextTool TextTool;
struct TextTool {
    Tool base;
};

TextTool* TextTool_Create();
void TextTool_Init(TextTool* pTextTool);

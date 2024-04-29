#include "../precomp.h"

#include "pointertool.h"

PointerTool* PointerTool_Create();
void PointerTool_Init(PointerTool* pPointerTool);

PointerTool* PointerTool_Create()
{
    PointerTool* pPointerTool = (PointerTool*)malloc(sizeof(PointerTool));
    memset(pPointerTool, 0, sizeof(PointerTool));
    PointerTool_Init(pPointerTool);
    return pPointerTool;
}

void PointerTool_Init(PointerTool* pPointerTool)
{
    pPointerTool->base.pszLabel = L"Pointer";
    pPointerTool->base.img = 0;
}

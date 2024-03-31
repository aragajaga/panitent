#include "common.h"

typedef struct HashMap HashMap;
extern HashMap g_hWndMap;

void WindowingInit()
{
    InitHashMap(&g_hWndMap, 127);
}

#include "common.h"

#include "window.h"
#include "../util/hashmap.h"

HashMap* g_pHWNDMap;

void WindowMap_Insert(HWND hWnd, Window* pWindow)
{
    if (g_pHWNDMap)
    {
        HashMap_Put(g_pHWNDMap, (void*)hWnd, (void*)pWindow);
    }
    else {
        MessageBox(NULL, L"WindowMap not initialized", NULL, MB_OK | MB_ICONERROR);
    }
}

Window* WindowMap_Get(HWND hWnd)
{
    Window* pWindow = NULL;

    if (g_pHWNDMap)
    {
        pWindow = HashMap_Get(g_pHWNDMap, (void*)hWnd);
    }
    else {
        MessageBox(NULL, L"WindowMap not initialized", NULL, MB_OK | MB_ICONERROR);
    }
    
    return pWindow;
}

/**
 * [PRIVATE]
 * 
 * Window system descriptor key comparison callback for global window hashmap
 * 
 * @return CMP_GREATER (1), CMP_LOWER (-1) or CMP_EQUAL (0)
 */
static int WindowMap_KeyCompare(void* pKey1, void* pKey2)
{
    return (HWND)pKey1 > (HWND)pKey2 ? CMP_GREATER : (HWND)pKey1 < (HWND)pKey2 ? CMP_LOWER : CMP_EQUAL;
}

void WindowMap_GlobalInit()
{
    g_pHWNDMap = HashMap_Create(16, &WindowMap_KeyCompare);
}

Window* pWndCreating;

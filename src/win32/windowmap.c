#include "common.h"

#include "window.h"
#include "../util/hashmap.h"

static int WindowMap_KeyCompare(void* pKey1, void* pKey2);

HashMap* WindowMap_GetWindowMap()
{
    static HashMap* s_pHWNDMap;
    if (!s_pHWNDMap)
    {
        s_pHWNDMap = HashMap_Create(16, &WindowMap_KeyCompare);
    }

    return s_pHWNDMap;
}

void WindowMap_Insert(HWND hWnd, Window* pWindow)
{
    HashMap* pWindowMap = WindowMap_GetWindowMap();
    HashMap_Put(pWindowMap, (void*)hWnd, (void*)pWindow);
}

Window* WindowMap_Get(HWND hWnd)
{
    HashMap* pWindowMap = WindowMap_GetWindowMap();

    Window* pWindow = NULL;
    pWindow = HashMap_Get(pWindowMap, (void*)hWnd);
    
    return pWindow;
}

void WindowMap_Erase(HWND hWnd)
{
    HashMap* pWindowMap = WindowMap_GetWindowMap();

    if (pWindowMap)
    {
        HashMap_Remove(pWindowMap, (void*)hWnd);
    }
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

Window* pWndCreating;

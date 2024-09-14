#ifndef _WIN32_WINDOWMAP_H_INCLUDED
#define _WIN32_WINDOWMAP_H_INCLUDED

typedef struct Window Window;

typedef struct HashMap HashMap;

void WindowMap_Insert(HWND hWnd, Window* pWindow);
Window* WindowMap_Get(HWND hWnd);
void WindowMap_Erase(HWND hWnd);

extern Window* pWndCreating;
extern HashMap* g_pHWNDMap;

#endif  // _WIN32_WINDOWMAP_H_INCLUDED

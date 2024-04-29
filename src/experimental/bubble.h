#pragma once

extern HWND g_hWndBubble;
extern POINT g_localCursorPos;

extern RECT g_rcWindow;
extern RECT g_rcClient;
extern RECT g_rcTemp;

BOOL Bubble_RegisterClass(HINSTANCE hInstance);
HWND Bubble_CreateWindow(HINSTANCE hInstance);

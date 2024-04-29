#pragma once

typedef struct Window Window;
typedef struct DockHostComposite DockHostComposite;

typedef struct DockWindowInfo DockWindowInfo;
struct DockWindowInfo {
    HWND hWnd;
};

DockHostComposite* DockHostComposite_Create();
void DockHostComposite_Init(DockHostComposite* pDockHostComposite);
void DockHostComposite_SetWindow(DockHostComposite* pDockHostComposite, Window* pWindow);
Window* DockHostComposite_GetWindow(DockHostComposite* pDockHostComposite);
LRESULT DockHostComposite_WndProc(DockHostComposite* pDockHostComposite, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, PBOOL pbProcessed);
void DockHostComposite_Dock(DockHostComposite* pDockHostComposite, DockWindowInfo* pWindowInfo, int nSide);
void DockHostComposite_Draw(DockHostComposite* pDockHostComposite, HDC hdc);
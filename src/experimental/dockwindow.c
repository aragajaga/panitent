#include "../precomp.h"

#include "dockwindow.h"
#include "dockpanel.h"

#include "../resource.h"

#include "../win32/window.h"

#include "dockhostcomposite.h"
#include "../util/assert.h"

static const WCHAR szClassName[] = L"Experimental::DockWindow";

typedef struct DockHostWindow2 DockHostWindow2;
struct DockHostWindow2 {
    Window base;
    DockHostComposite* pDockHostComposite;
};

DockHostWindow2* DockHostWindow2_Create(Application* pApplication);
void DockHostWindow2_Init(DockHostWindow2* pDockHostWindow2, Application* pApplication);
void DockHostWindow2_PreRegister(LPWNDCLASSEX pwcex);
void DockHostWindow2_PreCreate(LPCREATESTRUCT lpcs);
void DockHostWindow2_OnCreate(DockHostWindow2* pDockHostWindow2, LPCREATESTRUCT lpcs);
void DockHostWindow2_OnPaint(DockHostWindow2* pDockHostWindow2);
LRESULT DockHostWindow2_UserProc(DockHostWindow2* pDockHostWindow2, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DockHostWindow2* DockHostWindow2_Create(Application* pApplication)
{
    DockHostWindow2* pDockHostWindow2 = (DockHostWindow2*)malloc(sizeof(DockHostWindow2));

    if (pDockHostWindow2)
    {
        DockHostWindow2_Init(pDockHostWindow2, pApplication);
    }

    return pDockHostWindow2;
}

void DockHostWindow2_Init(DockHostWindow2* pDockHostWindow2, Application* pApplication)
{
    memset(pDockHostWindow2, 0, sizeof(DockHostWindow2));

    Window_Init(&pDockHostWindow2->base);

    _WindowInitHelper_SetPreCreateRoutine(pDockHostWindow2, (FnWindowPreCreate)DockHostWindow2_PreCreate);
    _WindowInitHelper_SetPreRegisterRoutine(pDockHostWindow2, (FnWindowPreRegister)DockHostWindow2_PreRegister);
    _WindowInitHelper_SetUserProcRoutine(pDockHostWindow2, (FnWindowUserProc)DockHostWindow2_UserProc);

    pDockHostWindow2->base.OnCreate = (FnWindowOnCreate)DockHostWindow2_OnCreate;
    pDockHostWindow2->base.OnPaint = (FnWindowOnPaint)DockHostWindow2_OnPaint;

    pDockHostWindow2->pDockHostComposite = DockHostComposite_Create();
}

void DockHostWindow2_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void DockHostWindow2_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"Panit.ent Alpha 0.4.2";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

void DockHostWindow2_OnCreate(DockHostWindow2* pDockHostWindow2, LPCREATESTRUCT lpcs)
{
    DockHostComposite_SetWindow(pDockHostWindow2->pDockHostComposite, pDockHostWindow2);

    DockWindowInfo dwi = { 0 };

    HWND hDockPanel1 = CreateWindowEx(WS_EX_TOOLWINDOW, L"Experimental::DockPanel", L"Toolbox", WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, Window_GetHWND(pDockHostWindow2), NULL, GetModuleHandle(NULL), NULL);
    ASSERT(hDockPanel1);
    dwi.hWnd = hDockPanel1;
    DockHostComposite_Dock(pDockHostWindow2->pDockHostComposite, &dwi, 1);

    HWND hDockPanel2 = CreateWindowEx(WS_EX_TOOLWINDOW, L"Experimental::DockPanel", L"GLWindow", WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, Window_GetHWND(pDockHostWindow2), NULL, GetModuleHandle(NULL), NULL);
    ASSERT(hDockPanel2);
    dwi.hWnd = hDockPanel2;
    DockHostComposite_Dock(pDockHostWindow2->pDockHostComposite, &dwi, 1);

    HWND hDockPanel3 = CreateWindowEx(WS_EX_TOOLWINDOW, L"Experimental::DockPanel", L"Info", WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, Window_GetHWND(pDockHostWindow2), NULL, GetModuleHandle(NULL), NULL);
    ASSERT(hDockPanel3);
    dwi.hWnd = hDockPanel3;
    DockHostComposite_Dock(pDockHostWindow2->pDockHostComposite, &dwi, 3);

    HWND hDockPanel4 = CreateWindowEx(WS_EX_TOOLWINDOW, L"Experimental::DockPanel", L"Palette", WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, Window_GetHWND(pDockHostWindow2), NULL, GetModuleHandle(NULL), NULL);
    ASSERT(hDockPanel4);
    dwi.hWnd = hDockPanel4;
    DockHostComposite_Dock(pDockHostWindow2->pDockHostComposite, &dwi, 3);

    HWND hDockPanel5 = CreateWindowEx(WS_EX_TOOLWINDOW, L"Experimental::DockPanel", L"Layers", WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, Window_GetHWND(pDockHostWindow2), NULL, GetModuleHandle(NULL), NULL);
    ASSERT(hDockPanel5);
    dwi.hWnd = hDockPanel5;
    DockHostComposite_Dock(pDockHostWindow2->pDockHostComposite, &dwi, 3);

    return TRUE;
}

void DockHostWindow2_OnPaint(DockHostWindow2* pDockHostWindow2)
{
    HWND hWnd = Window_GetHWND(pDockHostWindow2);
    RECT rcClient;
    Window_GetClientRect(pDockHostWindow2, &rcClient);
    
    PAINTSTRUCT ps = { 0 };
    HDC hdc = BeginPaint(hWnd, &ps);

    Rectangle(hdc, 0, 0, rcClient.right, 48);

    DockHostComposite_Draw(pDockHostWindow2->pDockHostComposite, hdc);

    EndPaint(hWnd, &ps);
}

LRESULT DockHostWindow2_UserProc(DockHostWindow2* pDockHostWindow2, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DockHostComposite* pDockHostComposite = pDockHostWindow2->pDockHostComposite;
    
    LRESULT lResult = 0;
    BOOL bProcessed = FALSE;
    if (pDockHostComposite)
    {
        lResult = DockHostComposite_WndProc(pDockHostComposite, hWnd, message, wParam, lParam, &bProcessed);
    }

    if (bProcessed)
    {
        return lResult;
    }

    return Window_UserProcDefault(pDockHostWindow2, hWnd, message, wParam, lParam);
}

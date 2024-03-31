#include "precomp.h"
#include "win32/window.h"
#include "win32/util.h"
#include "viewport.h"
#include "workspacecontainer.h"

static const WCHAR szClassName[] = L"__WorkspaceContainer";

/* Private forward declarations */
WorkspaceContainer* WorkspaceContainer_Create(struct Application*);
void WorkspaceContainer_Init(WorkspaceContainer*, struct Application*);

void WorkspaceContainer_PreRegister(LPWNDCLASSEX lpwcex);
void WorkspaceContainer_PreCreate(LPCREATESTRUCT lpcs);

BOOL WorkspaceContainer_OnCreate(WorkspaceContainer* pWorkspaceContainer, LPCREATESTRUCT lpcs);
void WorkspaceContainer_OnPaint(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_OnLButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnRButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnContextMenu(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags);
void WorkspaceContainer_OnSize(WorkspaceContainer* pWorkspaceContainer, UINT state, int cx, int cy);
void WorkspaceContainer_OnDestroy(WorkspaceContainer* pWorkspaceContainer);

LRESULT CALLBACK WorkspaceContainer_UserProc(WorkspaceContainer* pWorkspaceContainer, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct ViewportVector {
    size_t m_capacity;
    size_t m_size;
    ViewportWindow** pData;
};

ViewportVector* ViewportVector_Create();
void ViewportVector_Init(ViewportVector* pViewportVector);

ViewportVector* ViewportVector_Create()
{
    ViewportVector* pViewportVector = (ViewportVector *)calloc(1, sizeof(ViewportVector));
    ViewportVector_Init(pViewportVector);
    return pViewportVector;
}

void ViewportVector_Init(ViewportVector* pViewportVector)
{
    pViewportVector->m_capacity = 0;
    pViewportVector->m_size = 0;
    pViewportVector->pData = NULL;

    ViewportWindow** pVectorData = (ViewportWindow**)calloc(10, sizeof(ViewportWindow*));
    if (pVectorData)
    {
        pViewportVector->pData = pVectorData;
        pViewportVector->m_capacity = 10;
    }
}

size_t ViewportVector_GetSize(ViewportVector* pViewportVector)
{
    return pViewportVector->m_size;
}

void ViewportVector_Add(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow)
{
    if (pViewportVector && pViewportVector->pData)
    {
        pViewportVector->pData[pViewportVector->m_size++] = pViewportWindow;
    }
}

ViewportWindow* ViewportVector_Get(ViewportVector* pViewportVector, int idx)
{
    if (idx < pViewportVector->m_size)
    {
        return pViewportVector->pData[idx];
    }

    return NULL;
}

WorkspaceContainer* WorkspaceContainer_Create(struct Application* app)
{
    WorkspaceContainer* pWorkspaceContainer = calloc(1, sizeof(WorkspaceContainer));

    if (pWorkspaceContainer)
    {
        WorkspaceContainer_Init(pWorkspaceContainer, app);
    }

    return pWorkspaceContainer;
}

void WorkspaceContainer_Init(WorkspaceContainer* pWorkspaceContainer, struct Application* app)
{
    Window_Init(&pWorkspaceContainer->base, app);

    pWorkspaceContainer->base.szClassName = szClassName;

    pWorkspaceContainer->base.OnCreate = (FnWindowOnCreate)WorkspaceContainer_OnCreate;
    pWorkspaceContainer->base.OnDestroy = (FnWindowOnDestroy)WorkspaceContainer_OnDestroy;
    pWorkspaceContainer->base.OnPaint = (FnWindowOnPaint)WorkspaceContainer_OnPaint;
    pWorkspaceContainer->base.OnSize = (FnWindowOnSize)WorkspaceContainer_OnSize;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pWorkspaceContainer, (FnWindowPreRegister)WorkspaceContainer_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pWorkspaceContainer, (FnWindowPreCreate)WorkspaceContainer_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pWorkspaceContainer, (FnWindowUserProc)WorkspaceContainer_UserProc);

    pWorkspaceContainer->m_pViewportVector = ViewportVector_Create();
}

void WorkspaceContainer_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = szClassName;
}

void WorkspaceContainer_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"WorkspaceContainer";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

BOOL WorkspaceContainer_OnCreate(WorkspaceContainer* pWorkspaceContainer, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(lpcs);

    return TRUE;
}

void WorkspaceContainer_OnSize(WorkspaceContainer* pWorkspaceContainer, UINT state, int cx, int cy)
{
    if (pWorkspaceContainer->m_pViewportWindow)
    {
        HWND hWndViewport = Window_GetHWND((Window*)pWorkspaceContainer->m_pViewportWindow);

        RECT rcClient = { 0 };
        Window_GetClientRect(pWorkspaceContainer, &rcClient);

        SetWindowPos(hWndViewport, NULL, 0, 24, cx, cy - 24, 0);
    }
}

void WorkspaceContainer_DrawSingleTab(WorkspaceContainer* pWorkspaceContainer, HDC hdc)
{

}

void WorkspaceContainer_DrawTabs(WorkspaceContainer* pWorkspaceContainer, HDC hdc)
{
    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    RECT rcTabHeader;
    rcTabHeader.left = rcClient.left;
    rcTabHeader.top = rcClient.top;
    rcTabHeader.right = rcClient.right;
    rcTabHeader.bottom = rcClient.top + 24;

    HBRUSH hBrush = CreateSolidBrush(Win32_HexToCOLORREF(L"#9185be"));
    HBRUSH hOldBrush = SelectObject(hdc, hBrush);
    
    FillRect(hdc, &rcTabHeader, hBrush);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    

    for (int i = 0; i < ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        RECT rcTab;
        rcTab.left = rcTabHeader.left + i * 130;
        rcTab.top = rcTabHeader.top;
        rcTab.right = rcTabHeader.right;
        rcTab.bottom = rcTabHeader.bottom;

        Win32_ContractRect(&rcTab, 2, 2);

        rcTab.right = rcTab.left + 128;


        HBRUSH hBrush = CreateSolidBrush(Win32_HexToCOLORREF(L"#ada4ce"));
        FillRect(hdc, &rcTab, hBrush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));

        

        WCHAR szText[256] = L"";
        swprintf_s(szText, 256, L"Document %d", i);
        size_t len = wcslen(szText);
        DrawText(hdc, szText, len, &rcTab, DT_VCENTER);
    }
}

void WorkspaceContainer_OnPaint(WorkspaceContainer* pWorkspaceContainer)
{
    HWND hwnd = pWorkspaceContainer->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    WorkspaceContainer_DrawTabs(pWorkspaceContainer, hdc);

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void WorkspaceContainer_OnLButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WorkspaceContainer_OnRButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WorkspaceContainer_OnContextMenu(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void WorkspaceContainer_OnCommand(WorkspaceContainer* pWorkspaceContainer, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
}

void WorkspaceContainer_OnDestroy(WorkspaceContainer* pWorkspaceContainer)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
}

LRESULT CALLBACK WorkspaceContainer_UserProc(WorkspaceContainer* pWorkspaceContainer, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_RBUTTONUP:
        WorkspaceContainer_OnRButtonUp(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        return TRUE;
        break;

    case WM_LBUTTONUP:
        WorkspaceContainer_OnLButtonUp(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        WorkspaceContainer_OnContextMenu(pWorkspaceContainer, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        break;
    }

    return Window_UserProcDefault(pWorkspaceContainer, hWnd, message, wParam, lParam);
}

void WorkspaceContainer_AddViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow)
{
    HWND hWndWorkspaceContainer = Window_GetHWND(pWorkspaceContainer);
    HWND hWndViewport = Window_GetHWND(pViewportWindow);

    /* Set viewport parent to workspace container */
    SetParent(hWndViewport, hWndWorkspaceContainer);

    /* Set child window style for viewport window */
    DWORD dwStyle = Window_GetStyle(pViewportWindow);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    Window_SetStyle(pViewportWindow, dwStyle);

    /* Add viewport to viewports list */
    ViewportVector_Add(pWorkspaceContainer->m_pViewportVector, pViewportWindow);

    /* Set _current_ viewport window to the last one */
    pWorkspaceContainer->m_pViewportWindow = pViewportWindow;
}

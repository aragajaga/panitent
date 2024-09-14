#include "precomp.h"
#include "win32/window.h"

#include "splittercontainer.h"

static const WCHAR szClassName[] = L"__SplitterContainer";

/* Private forward declarations */
SplitterContainer* SplitterContainer_Create();
void SplitterContainer_Init(SplitterContainer*);

void SplitterContainer_PreRegister(LPWNDCLASSEX);
void SplitterContainer_PreCreate(LPCREATESTRUCT);

BOOL SplitterContainer_OnCreate(SplitterContainer*, LPCREATESTRUCT);
void SplitterContainer_OnPaint(SplitterContainer*);
void SplitterContainer_OnLButtonUp(SplitterContainer*, int, int);
void SplitterContainer_OnRButtonUp(SplitterContainer*, int, int);
void SplitterContainer_OnContextMenu(SplitterContainer*, int, int);
void SplitterContainer_OnDestroy(SplitterContainer*);
void SplitterContainer_OnSize(SplitterContainer* window, UINT state, int x, int y);
LRESULT CALLBACK SplitterContainer_UserProc(SplitterContainer* pSplitterContainer, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

SplitterContainer* SplitterContainer_Create()
{
    SplitterContainer* pWindow = (SplitterContainer*)malloc(sizeof(SplitterContainer));
    memset(pWindow, 0, sizeof(SplitterContainer));

    if (pWindow)
    {
        SplitterContainer_Init(pWindow);
    }

    return pWindow;
}

void SplitterContainer_Init(SplitterContainer* window)
{
    Window_Init(&window->base);

    window->base.szClassName = szClassName;

    window->base.OnCreate = SplitterContainer_OnCreate;
    window->base.OnDestroy = SplitterContainer_OnDestroy;
    window->base.OnPaint = SplitterContainer_OnPaint;
    window->base.OnSize = SplitterContainer_OnSize;
    window->base.PreRegister = SplitterContainer_PreRegister;
    window->base.PreCreate = SplitterContainer_PreCreate;
    window->base.UserProc = SplitterContainer_UserProc;

    window->iGripPos = 128;
    window->bVertical = FALSE;
}

void SplitterContainer_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE);
    lpwcex->lpszClassName = szClassName;
}

BOOL SplitterContainer_OnCreate(SplitterContainer* window, LPCREATESTRUCT lpcs)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lpcs);

    return TRUE;
}

int g_gripWidth = 6;

typedef struct KeyValuePair KeyValuePair;

struct KeyValuePair {
    void* key;
    void* value;
};

typedef struct HashMap2 HashMap2;

struct HashMap2 {
    struct KeyValuePair* pairs;
    size_t capacity;
    size_t size;
};

void InitHashMap2(struct HashMap2* map, size_t capacity)
{
    map->pairs = (struct KeyValuePair*)malloc(capacity * sizeof(KeyValuePair));
    memset(map->pairs, 0, capacity * sizeof(KeyValuePair));
    map->capacity = capacity;
    map->size = 0;
}

void InsertIntoHashMap2(struct HashMap2* map, void* key, void* value)
{
    if (map->size < map->capacity)
    {
        size_t index = map->size;
        map->pairs[index].key = key;
        map->pairs[index].value = value;
        map->size++;
    }
}

void* GetFromHashMap2(struct HashMap2* map, void* key)
{
    for (size_t i = 0; i < map->size; ++i)
    {
        if (map->pairs[i].key == key)
        {
            return map->pairs[i].value;
        }
    }
    return NULL;
}

void FreeHashMap2(struct HashMap2* map)
{
    free(map->pairs);
    map->capacity = 0;
    map->size = 0;
}

HashMap2 g_brushHashMap;

void InitBrushStorage()
{
    InitHashMap2(&g_brushHashMap, 16);
}

HBRUSH Win32_GetSolidBrush(COLORREF color)
{
    HBRUSH hBrush = GetFromHashMap2(&g_brushHashMap, color);

    if (hBrush)
    {
        return hBrush;
    }
    else {
        HBRUSH hBrush = CreateSolidBrush(color);
        InsertIntoHashMap2(&g_brushHashMap, color, hBrush);
        return hBrush;
    }
}

void SplitterContainer_GetSplitRect1(SplitterContainer* window, LPRECT pRect)
{

}

void SplitterContainer_GetSplitRect2(SplitterContainer* window, LPRECT pRect)
{
    HWND hWnd = Window_GetHWND(window);

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
}

void SplitterContainer_OnPaint(SplitterContainer* window)
{
    HWND hwnd = window->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    RECT rcClient;
    GetClientRect(window->base.hWnd, &rcClient);

    HBRUSH hBrush = Win32_GetSolidBrush(RGB(0xFF, 0x00, 0x00));

    SelectObject(hdc, hBrush);
    if (window->bSplitterCaptured)
    {
        SetDCBrushColor(hdc, RGB(255, 0, 0));
    }

    if (window->bVertical)
    {
        Rectangle(hdc, 0, window->iGripPos - g_gripWidth / 2, rcClient.right, window->iGripPos + g_gripWidth / 2);
    }
    else {
        Rectangle(hdc, window->iGripPos - g_gripWidth / 2, 0, window->iGripPos + g_gripWidth / 2, rcClient.bottom);
    }

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void SplitterContainer_OnRButtonUp(SplitterContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void SplitterContainer_OnContextMenu(SplitterContainer* window, int x, int y)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void SplitterContainer_OnCommand(SplitterContainer* window, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(window);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
}

void SplitterContainer_OnDestroy(SplitterContainer* window)
{
    UNREFERENCED_PARAMETER(window);
}

void SplitterContainer_UpdateChildLayout1(SplitterContainer* pSplitterContainer)
{
    const int iGripPos = pSplitterContainer->iGripPos;

    RECT rcClient;
    GetClientRect(pSplitterContainer->base.hWnd, &rcClient);

    if (pSplitterContainer->hWnd1 && IsWindow(pSplitterContainer->hWnd1))
    {
        if (IsChild(Window_GetHWND(pSplitterContainer), pSplitterContainer->hWnd1))
        {
            if (pSplitterContainer->bVertical)
            {
                SetWindowPos(pSplitterContainer->hWnd1, NULL, 0, 0, rcClient.right , iGripPos - g_gripWidth / 2, SWP_NOACTIVATE);
            }
            else {
                SetWindowPos(pSplitterContainer->hWnd1, NULL, 0, 0, iGripPos - g_gripWidth / 2, rcClient.bottom, SWP_NOACTIVATE);
            }
        }
    }
}

void SplitterContainer_UpdateChildLayout2(SplitterContainer* pSplitterContainer)
{
    const int iGripPos = pSplitterContainer->iGripPos;

    RECT rcClient;
    GetClientRect(pSplitterContainer->base.hWnd, &rcClient);

    if (pSplitterContainer->hWnd2 && IsWindow(pSplitterContainer->hWnd2))
    {
        if (IsChild(Window_GetHWND(pSplitterContainer), pSplitterContainer->hWnd1))
        {
            if (pSplitterContainer->bVertical)
            {
                SetWindowPos(pSplitterContainer->hWnd2, NULL, 0, iGripPos + g_gripWidth / 2, rcClient.right, rcClient.bottom - iGripPos - g_gripWidth / 2, SWP_NOACTIVATE);
            }
            else {
                SetWindowPos(pSplitterContainer->hWnd2, NULL, iGripPos + g_gripWidth / 2, 0, rcClient.right - iGripPos - g_gripWidth / 2, rcClient.bottom, SWP_NOACTIVATE);
            }
        }
    }
}

void SplitterContainer_OnSize(SplitterContainer* window, UINT state, int x, int y)
{
    SplitterContainer_UpdateChildLayout1(window);
    SplitterContainer_UpdateChildLayout2(window);
}

void SplitterContainer_OnMouseMove(SplitterContainer* window, int x, int y, UINT keyFlags)
{
    RECT rcClient;
    GetClientRect(window->base.hWnd, &rcClient);

    if (window->bSplitterCaptured)
    {
        window->iGripPos = window->bVertical ? y : x;
        SplitterContainer_UpdateChildLayout1(window);
        SplitterContainer_UpdateChildLayout2(window);

        InvalidateRect(window->base.hWnd, NULL, TRUE);
    }
    
    if (SplitterContainer_GripHitTest(window, x, y))
    {
        SetCursor(LoadCursor(NULL, window->bVertical ? IDC_SIZENS : IDC_SIZEWE));
    }
}

BOOL SplitterContainer_GripHitTest(SplitterContainer* window, int x, int y)
{
    int z = window->bVertical ? y : x;

    return (z >= window->iGripPos - g_gripWidth / 2 &&
        z < window->iGripPos + g_gripWidth / 2) ? TRUE : FALSE;
}

void SplitterContainer_OnLButtonDown(SplitterContainer* window, int x, int y, UINT keyFlags)
{
    RECT rcClient;
    GetClientRect(window->base.hWnd, &rcClient);

    if (SplitterContainer_GripHitTest(window, x, y))
    {
        SetCursor(LoadCursor(NULL, window->bVertical ? IDC_SIZENS : IDC_SIZEWE));
        SetCapture(window->base.hWnd);
        window->bSplitterCaptured = TRUE;
        InvalidateRect(window->base.hWnd, NULL, TRUE);
    }
}

void SplitterContainer_OnLButtonUp(SplitterContainer* window, int x, int y)
{
    if (window->bSplitterCaptured)
    {
        ReleaseCapture();
        window->bSplitterCaptured = FALSE;
        InvalidateRect(window->base.hWnd, NULL, TRUE);
    }
}

LRESULT CALLBACK SplitterContainer_UserProc(SplitterContainer* window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_MOUSEMOVE:
        SplitterContainer_OnMouseMove(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        return TRUE;
        break;

    case WM_RBUTTONUP:
        SplitterContainer_OnRButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_LBUTTONDOWN:
        SplitterContainer_OnLButtonDown(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT)wParam);
        return TRUE;
        break;

    case WM_LBUTTONUP:
        SplitterContainer_OnLButtonUp(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;

    case WM_CONTEXTMENU:
        SplitterContainer_OnContextMenu(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }

    return Window_UserProcDefault(window, hWnd, message, wParam, lParam);
}

void SplitterContainer_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"SplitterContainer";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 300;
    lpcs->cy = 200;
}

void SplitterContainer_PinWindow1(SplitterContainer* pSplitterContainer, HWND hWnd)
{
    pSplitterContainer->hWnd1 = hWnd;
    SetParent(hWnd, pSplitterContainer->base.hWnd);

    RECT rcClient;
    GetClientRect(pSplitterContainer->base.hWnd, &rcClient);

    DWORD dwStyle = GetWindowStyle(hWnd);
    dwStyle &= ~(WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);

    SplitterContainer_UpdateChildLayout1(pSplitterContainer);
}

void SplitterContainer_PinWindow2(SplitterContainer* pSplitterContainer, HWND hWnd)
{
    pSplitterContainer->hWnd2 = hWnd;
    SetParent(hWnd, pSplitterContainer->base.hWnd);

    RECT rcClient;
    GetClientRect(pSplitterContainer->base.hWnd, &rcClient);

    DWORD dwStyle = GetWindowStyle(hWnd);
    dwStyle &= ~(WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);

    SplitterContainer_UpdateChildLayout2(pSplitterContainer);
}

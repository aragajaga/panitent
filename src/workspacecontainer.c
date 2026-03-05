#include "precomp.h"
#include "win32/window.h"
#include "win32/util.h"
#include "viewport.h"
#include "workspacecontainer.h"
#include "document.h"
#include "panitentapp.h"

static const WCHAR szClassName[] = L"__WorkspaceContainer";

#define WORKSPACE_TAB_HEIGHT 24
#define WORKSPACE_TAB_GAP 2
#define WORKSPACE_TAB_TEXT_PADDING_X 10
#define WORKSPACE_TAB_MIN_WIDTH 72
#define WORKSPACE_TAB_MAX_WIDTH 240

/* Private forward declarations */
WorkspaceContainer* WorkspaceContainer_Create();
void WorkspaceContainer_Init(WorkspaceContainer*);

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

int WorkspaceContainer_GetTabHeaderHeight(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_LayoutViewports(WorkspaceContainer* pWorkspaceContainer);
void WorkspaceContainer_SetCurrentViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, BOOL bSyncAppState);
void WorkspaceContainer_GetViewportTitle(ViewportWindow* pViewportWindow, int nTabIndex, LPWSTR pszBuffer, size_t cchBuffer);
int WorkspaceContainer_CalcTabWidth(WorkspaceContainer* pWorkspaceContainer, HDC hdc, ViewportWindow* pViewportWindow, int nTabIndex);
BOOL WorkspaceContainer_GetTabRect(WorkspaceContainer* pWorkspaceContainer, HDC hdc, int nTabIndex, LPRECT prcTab);
int WorkspaceContainer_HitTestTab(WorkspaceContainer* pWorkspaceContainer, int x, int y);
int ViewportVector_FindIndex(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow);

struct ViewportVector {
    size_t m_capacity;
    size_t m_size;
    ViewportWindow** pData;
};

ViewportVector* ViewportVector_Create();
void ViewportVector_Init(ViewportVector* pViewportVector);
BOOL ViewportVector_Reserve(ViewportVector* pViewportVector, size_t newCapacity);
BOOL ViewportVector_Add(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow);
size_t ViewportVector_GetSize(ViewportVector* pViewportVector);
ViewportWindow* ViewportVector_Get(ViewportVector* pViewportVector, int idx);

ViewportVector* ViewportVector_Create()
{
    ViewportVector* pViewportVector = (ViewportVector *)malloc(sizeof(ViewportVector));
    if (!pViewportVector)
    {
        return NULL;
    }

    memset(pViewportVector, 0, sizeof(ViewportVector));
    ViewportVector_Init(pViewportVector);
    return pViewportVector;
}

void ViewportVector_Init(ViewportVector* pViewportVector)
{
    if (!pViewportVector)
    {
        return;
    }

    pViewportVector->m_capacity = 0;
    pViewportVector->m_size = 0;
    pViewportVector->pData = NULL;

    ViewportVector_Reserve(pViewportVector, 10);
}

WorkspaceContainer* WorkspaceContainer_Create()
{
    WorkspaceContainer* pWorkspaceContainer = (WorkspaceContainer*)malloc(sizeof(WorkspaceContainer));
    memset(pWorkspaceContainer, 0, sizeof(WorkspaceContainer));

    if (pWorkspaceContainer)
    {
        WorkspaceContainer_Init(pWorkspaceContainer);
    }

    return pWorkspaceContainer;
}

void WorkspaceContainer_Init(WorkspaceContainer* pWorkspaceContainer)
{
    Window_Init(&pWorkspaceContainer->base);

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
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);

    WorkspaceContainer_LayoutViewports(pWorkspaceContainer);
}

int WorkspaceContainer_GetTabHeaderHeight(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return 0;
    }

    return ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector) > 0 ?
        WORKSPACE_TAB_HEIGHT : 0;
}

void WorkspaceContainer_LayoutViewports(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    int top = WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer);
    int width = max(0, rcClient.right - rcClient.left);
    int height = max(0, rcClient.bottom - top);

    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, i);
        if (!pViewportWindow)
        {
            continue;
        }

        HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
        if (!IsWindow(hWndViewport))
        {
            continue;
        }

        if (pViewportWindow == pWorkspaceContainer->m_pViewportWindow)
        {
            SetWindowPos(hWndViewport, HWND_TOP, 0, top, width, height,
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
            continue;
        }

        ShowWindow(hWndViewport, SW_HIDE);
    }
}

void WorkspaceContainer_SetCurrentViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow, BOOL bSyncAppState)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector)
    {
        return;
    }

    if (!pViewportWindow && ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector) > 0)
    {
        pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, 0);
    }

    pWorkspaceContainer->m_pViewportWindow = pViewportWindow;
    WorkspaceContainer_LayoutViewports(pWorkspaceContainer);

    if (pViewportWindow)
    {
        HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);
        if (IsWindow(hWndViewport))
        {
            SetFocus(hWndViewport);
        }
    }

    if (bSyncAppState)
    {
        PanitentApp_SetActiveViewport(PanitentApp_Instance(), pViewportWindow);
    }

    Window_Invalidate((Window*)pWorkspaceContainer);
}

void WorkspaceContainer_GetViewportTitle(ViewportWindow* pViewportWindow, int nTabIndex, LPWSTR pszBuffer, size_t cchBuffer)
{
    if (!pszBuffer || cchBuffer == 0)
    {
        return;
    }

    StringCchPrintfW(pszBuffer, cchBuffer, L"Untitled %d", nTabIndex + 1);

    if (!pViewportWindow)
    {
        return;
    }

    Document* pDocument = ViewportWindow_GetDocument(pViewportWindow);
    if (!pDocument || !pDocument->szFilePath || !pDocument->szFilePath[0])
    {
        return;
    }

    PCWSTR pszFileName = PathFindFileNameW(pDocument->szFilePath);
    if (pszFileName && pszFileName[0])
    {
        StringCchCopyW(pszBuffer, cchBuffer, pszFileName);
    }
}

int WorkspaceContainer_CalcTabWidth(WorkspaceContainer* pWorkspaceContainer, HDC hdc, ViewportWindow* pViewportWindow, int nTabIndex)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);

    WCHAR szTitle[MAX_PATH] = L"";
    WorkspaceContainer_GetViewportTitle(pViewportWindow, nTabIndex, szTitle, ARRAYSIZE(szTitle));

    SIZE textExtent = { 0 };
    GetTextExtentPoint32W(hdc, szTitle, (int)wcslen(szTitle), &textExtent);

    int width = textExtent.cx + WORKSPACE_TAB_TEXT_PADDING_X * 2;
    width = max(width, WORKSPACE_TAB_MIN_WIDTH);
    width = min(width, WORKSPACE_TAB_MAX_WIDTH);
    return width;
}

BOOL WorkspaceContainer_GetTabRect(WorkspaceContainer* pWorkspaceContainer, HDC hdc, int nTabIndex, LPRECT prcTab)
{
    if (!pWorkspaceContainer || !prcTab || !hdc || nTabIndex < 0)
    {
        return FALSE;
    }

    size_t nTabs = ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector);
    if (nTabs == 0 || (size_t)nTabIndex >= nTabs)
    {
        return FALSE;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    RECT rcTabHeader = {
        rcClient.left,
        rcClient.top,
        rcClient.right,
        rcClient.top + WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer)
    };

    int x = rcTabHeader.left + WORKSPACE_TAB_GAP;
    for (int i = 0; i < (int)nTabs; i++)
    {
        ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, i);
        int tabWidth = WorkspaceContainer_CalcTabWidth(pWorkspaceContainer, hdc, pViewportWindow, i);

        RECT rcTab = {
            x,
            rcTabHeader.top + WORKSPACE_TAB_GAP,
            min(x + tabWidth, rcTabHeader.right - WORKSPACE_TAB_GAP),
            rcTabHeader.bottom
        };

        if (i == nTabIndex)
        {
            *prcTab = rcTab;
            return TRUE;
        }

        x += tabWidth + WORKSPACE_TAB_GAP;
        if (x >= rcTabHeader.right)
        {
            break;
        }
    }

    return FALSE;
}

int WorkspaceContainer_HitTestTab(WorkspaceContainer* pWorkspaceContainer, int x, int y)
{
    if (!pWorkspaceContainer)
    {
        return -1;
    }

    int headerHeight = WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer);
    if (headerHeight <= 0 || y < 0 || y >= headerHeight)
    {
        return -1;
    }

    HWND hWndWorkspaceContainer = Window_GetHWND((Window*)pWorkspaceContainer);
    HDC hdc = GetDC(hWndWorkspaceContainer);
    if (!hdc)
    {
        return -1;
    }

    HFONT hFont = PanitentApp_GetUIFont(PanitentApp_Instance());
    HGDIOBJ hPrevFont = SelectObject(hdc, hFont);

    int nHitTab = -1;
    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        RECT rcTab = { 0 };
        POINT pt = { x, y };
        if (WorkspaceContainer_GetTabRect(pWorkspaceContainer, hdc, i, &rcTab) &&
            PtInRect(&rcTab, pt))
        {
            nHitTab = i;
            break;
        }
    }

    SelectObject(hdc, hPrevFont);
    ReleaseDC(hWndWorkspaceContainer, hdc);
    return nHitTab;
}

void WorkspaceContainer_DrawSingleTab(WorkspaceContainer* pWorkspaceContainer, HDC hdc)
{
    UNREFERENCED_PARAMETER(pWorkspaceContainer);
    UNREFERENCED_PARAMETER(hdc);
}

void WorkspaceContainer_DrawTabs(WorkspaceContainer* pWorkspaceContainer, HDC hdc)
{
    if (WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer) == 0)
    {
        return;
    }

    RECT rcClient = { 0 };
    Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);

    RECT rcTabHeader = {
        rcClient.left,
        rcClient.top,
        rcClient.right,
        rcClient.top + WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer)
    };

    HBRUSH hHeaderBrush = CreateSolidBrush(Win32_HexToCOLORREF(L"#7d6cae"));
    FillRect(hdc, &rcTabHeader, hHeaderBrush);
    DeleteObject(hHeaderBrush);

    HFONT hUIFont = PanitentApp_GetUIFont(PanitentApp_Instance());
    HGDIOBJ hPrevFont = SelectObject(hdc, hUIFont);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < (int)ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector); i++)
    {
        RECT rcTab = { 0 };
        if (!WorkspaceContainer_GetTabRect(pWorkspaceContainer, hdc, i, &rcTab))
        {
            continue;
        }

        ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, i);
        BOOL bActive = pViewportWindow == pWorkspaceContainer->m_pViewportWindow;

        HBRUSH hTabBrush = CreateSolidBrush(
            bActive ? Win32_HexToCOLORREF(L"#ada4ce") : Win32_HexToCOLORREF(L"#9185be"));
        FillRect(hdc, &rcTab, hTabBrush);
        DeleteObject(hTabBrush);

        FrameRect(hdc, &rcTab, GetStockObject(BLACK_BRUSH));

        WCHAR szTitle[MAX_PATH] = L"";
        WorkspaceContainer_GetViewportTitle(pViewportWindow, i, szTitle, ARRAYSIZE(szTitle));

        RECT rcTabText = rcTab;
        rcTabText.left += WORKSPACE_TAB_TEXT_PADDING_X;
        rcTabText.right -= WORKSPACE_TAB_TEXT_PADDING_X;

        SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
        DrawTextW(hdc, szTitle, -1, &rcTabText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
    }

    SelectObject(hdc, hPrevFont);
}

void WorkspaceContainer_OnPaint(WorkspaceContainer* pWorkspaceContainer)
{
    HWND hwnd = pWorkspaceContainer->base.hWnd;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    WorkspaceContainer_DrawTabs(pWorkspaceContainer, hdc);

    if (!ViewportVector_GetSize(pWorkspaceContainer->m_pViewportVector))
    {
        RECT rcClient;
        Window_GetClientRect((Window*)pWorkspaceContainer, &rcClient);
        rcClient.top += WorkspaceContainer_GetTabHeaderHeight(pWorkspaceContainer);
        ContractRect(&rcClient, 24);

        HFONT hFont = PanitentApp_GetUIFont(PanitentApp_Instance());
        LOGFONT lf;
        GetObject(hFont, sizeof(lf), &lf);
        lf.lfHeight = 24;
        HFONT hWelcomeFont = CreateFontIndirect(&lf);

        HFONT hPrevFont = (HFONT)SelectObject(hdc, hWelcomeFont);

        SetBkMode(hdc, TRANSPARENT);
        WCHAR szWelcomeText[] = L"Welcome to Panit.ent! Create a new or open an existing file using the File menu";
        DrawTextEx(hdc, szWelcomeText, (int)wcslen(szWelcomeText), &rcClient, 0, NULL);

        SelectObject(hdc, hPrevFont);
        DeleteObject(hWelcomeFont);
        
    }
    

    /* End painting the window */
    EndPaint(hwnd, &ps);
}

void WorkspaceContainer_OnLButtonUp(WorkspaceContainer* pWorkspaceContainer, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    int nHitTab = WorkspaceContainer_HitTestTab(pWorkspaceContainer, x, y);
    if (nHitTab < 0)
    {
        return;
    }

    ViewportWindow* pViewportWindow = ViewportVector_Get(pWorkspaceContainer->m_pViewportVector, nHitTab);
    if (!pViewportWindow)
    {
        return;
    }

    WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pViewportWindow, TRUE);
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

    return Window_UserProcDefault((Window*)pWorkspaceContainer, hWnd, message, wParam, lParam);
}

void WorkspaceContainer_AddViewport(WorkspaceContainer* pWorkspaceContainer, ViewportWindow* pViewportWindow)
{
    if (!pWorkspaceContainer || !pWorkspaceContainer->m_pViewportVector || !pViewportWindow)
    {
        return;
    }

    int nExistingIndex = ViewportVector_FindIndex(pWorkspaceContainer->m_pViewportVector, pViewportWindow);
    if (nExistingIndex >= 0)
    {
        WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pViewportWindow, TRUE);
        return;
    }

    HWND hWndWorkspaceContainer = Window_GetHWND((Window*)pWorkspaceContainer);
    HWND hWndViewport = Window_GetHWND((Window*)pViewportWindow);

    /* Set viewport parent to workspace container */
    if (IsWindow(hWndViewport))
    {
        SetParent(hWndViewport, hWndWorkspaceContainer);
    }

    /* Set child window style for viewport window */
    DWORD dwStyle = Window_GetStyle((Window*)pViewportWindow);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME);
    dwStyle |= WS_CHILD;
    Window_SetStyle((Window*)pViewportWindow, dwStyle);
    SetWindowPos(hWndViewport, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    /* Add viewport to viewports list */
    if (!ViewportVector_Add(pWorkspaceContainer->m_pViewportVector, pViewportWindow))
    {
        return;
    }

    /* Activate new viewport tab */
    WorkspaceContainer_SetCurrentViewport(pWorkspaceContainer, pViewportWindow, TRUE);
}

ViewportWindow* WorkspaceContainer_GetCurrentViewport(WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return NULL;
    }

    return pWorkspaceContainer->m_pViewportWindow;
}

BOOL ViewportVector_Reserve(ViewportVector* pViewportVector, size_t newCapacity)
{
    if (!pViewportVector)
    {
        return FALSE;
    }

    if (newCapacity <= pViewportVector->m_capacity)
    {
        return TRUE;
    }

    ViewportWindow** pNewData = (ViewportWindow**)realloc(
        pViewportVector->pData, newCapacity * sizeof(ViewportWindow*));
    if (!pNewData)
    {
        return FALSE;
    }

    memset(
        pNewData + pViewportVector->m_capacity,
        0,
        (newCapacity - pViewportVector->m_capacity) * sizeof(ViewportWindow*));

    pViewportVector->pData = pNewData;
    pViewportVector->m_capacity = newCapacity;
    return TRUE;
}

size_t ViewportVector_GetSize(ViewportVector* pViewportVector)
{
    if (!pViewportVector)
    {
        return 0;
    }

    return pViewportVector->m_size;
}

BOOL ViewportVector_Add(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow)
{
    if (!pViewportVector || !pViewportWindow)
    {
        return FALSE;
    }

    if (pViewportVector->m_size >= pViewportVector->m_capacity)
    {
        size_t newCapacity = pViewportVector->m_capacity > 0 ?
            pViewportVector->m_capacity * 2 : 10;
        if (!ViewportVector_Reserve(pViewportVector, newCapacity))
        {
            return FALSE;
        }
    }

    pViewportVector->pData[pViewportVector->m_size++] = pViewportWindow;
    return TRUE;
}

ViewportWindow* ViewportVector_Get(ViewportVector* pViewportVector, int idx)
{
    if (!pViewportVector || idx < 0 || (size_t)idx >= pViewportVector->m_size)
    {
        return NULL;
    }

    return pViewportVector->pData[idx];
}

int ViewportVector_FindIndex(ViewportVector* pViewportVector, ViewportWindow* pViewportWindow)
{
    if (!pViewportVector || !pViewportWindow)
    {
        return -1;
    }

    for (int i = 0; i < (int)pViewportVector->m_size; i++)
    {
        if (pViewportVector->pData[i] == pViewportWindow)
        {
            return i;
        }
    }

    return -1;
}

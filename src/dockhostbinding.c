#include "precomp.h"

#include "dockhostbinding.h"

#include "dockgroup.h"
#include "dockviewcatalog.h"
#include "win32/window.h"
#include "win32/windowmap.h"

static void DockHostBinding_AssignPersistentNameForHWND(DockData* pDockData, HWND hWnd)
{
    WCHAR szClassNameBuf[64] = L"";
    WCHAR szTitleBuf[MAX_PATH] = L"";
    PanitentDockViewId nViewId;
    PCWSTR pszCanonicalName;

    if (!pDockData || !hWnd || !IsWindow(hWnd))
    {
        return;
    }

    GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
    GetWindowTextW(hWnd, szTitleBuf, ARRAYSIZE(szTitleBuf));
    nViewId = PanitentDockViewCatalog_FindForWindow(szClassNameBuf, szTitleBuf);
    pszCanonicalName = PanitentDockViewCatalog_GetCanonicalName(nViewId);
    if (pszCanonicalName && pszCanonicalName[0] != L'\0')
    {
        pDockData->nViewId = nViewId;
        wcscpy_s(pDockData->lpszName, MAX_PATH, pszCanonicalName);
        return;
    }

    if (pDockData->lpszCaption[0] != L'\0')
    {
        wcscpy_s(pDockData->lpszName, MAX_PATH, pDockData->lpszCaption);
    }
}

BOOL DockHostWindow_IsWorkspaceWindow(HWND hWnd)
{
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    WCHAR szClassNameBuf[64] = L"";
    GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
    return wcscmp(szClassNameBuf, L"__WorkspaceContainer") == 0;
}

DockPaneKind DockHostWindow_DeterminePaneKindForHWND(HWND hWnd)
{
    WCHAR szClassNameBuf[64] = L"";
    if (!hWnd || !IsWindow(hWnd))
    {
        return DOCK_PANE_NONE;
    }

    if (DockHostWindow_IsWorkspaceWindow(hWnd))
    {
        return DOCK_PANE_DOCUMENT;
    }

    GetClassNameW(hWnd, szClassNameBuf, ARRAYSIZE(szClassNameBuf));
    if (wcscmp(szClassNameBuf, L"__DockHostWindow") == 0)
    {
        Window* pWindow = WindowMap_Get(hWnd);
        DockHostWindow* pDockHostWindow = (DockHostWindow*)pWindow;
        TreeNode* pRoot = pDockHostWindow ? DockHostWindow_GetRoot(pDockHostWindow) : NULL;
        if (DockGroup_NodeContainsPaneKind(pRoot, DOCK_PANE_DOCUMENT))
        {
            return DOCK_PANE_DOCUMENT;
        }
    }

    return DOCK_PANE_TOOL;
}

void DockData_PinHWND(DockHostWindow* pDockHostWindow, DockData* pDockData, HWND hWnd)
{
    if (!pDockHostWindow || !pDockData || !hWnd || !IsWindow(hWnd))
    {
        return;
    }

    GetWindowText(hWnd, pDockData->lpszCaption, MAX_PATH);
    SetParent(hWnd, Window_GetHWND((Window*)pDockHostWindow));

    DWORD dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    dwStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_POPUP);
    dwStyle |= WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);

    pDockData->bShowCaption = !DockHostWindow_IsWorkspaceWindow(hWnd);
    pDockData->hWnd = hWnd;
    if (DockHostWindow_IsWorkspaceWindow(hWnd))
    {
        pDockData->nRole = DOCK_ROLE_WORKSPACE;
        pDockData->nPaneKind = DOCK_PANE_DOCUMENT;
    }
    else if (pDockData->nRole == DOCK_ROLE_UNKNOWN)
    {
        pDockData->nRole = DOCK_ROLE_PANEL;
        pDockData->nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    }
    else if (pDockData->nPaneKind == DOCK_PANE_NONE)
    {
        pDockData->nPaneKind = DockHostWindow_DeterminePaneKindForHWND(hWnd);
    }

    if (pDockData->lpszName[0] == L'\0')
    {
        DockHostBinding_AssignPersistentNameForHWND(pDockData, hWnd);
    }
}

void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window)
{
    HWND hWnd = Window_GetHWND(window);
    DockData_PinHWND(pDockHostWindow, pDockData, hWnd);
}

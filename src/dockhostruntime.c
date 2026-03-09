#include "precomp.h"

#include "dockhostruntime.h"

#include "dockgroup.h"
#include "dockhostautohide.h"
#include "dockhostinput.h"
#include "dockinspectordialog.h"
#include "dockviewcatalog.h"
#include "theme.h"
#include "win32/window.h"
#include "win32/windowmap.h"

static BOOL DockHostRuntime_ShouldPreserveHwnd(HWND hWnd, const HWND* phWndPreserve, int cPreserve)
{
    if (!hWnd || !phWndPreserve || cPreserve <= 0)
    {
        return FALSE;
    }

    for (int i = 0; i < cPreserve; ++i)
    {
        if (phWndPreserve[i] == hWnd)
        {
            return TRUE;
        }
    }

    return FALSE;
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

static void DockHostRuntime_AssignPersistentNameForHWND(DockData* pDockData, HWND hWnd)
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

static void DockHostRuntime_DestroyTreeRecursive(TreeNode* pNode, const HWND* phWndPreserve, int cPreserve)
{
    if (!pNode)
    {
        return;
    }

    DockHostRuntime_DestroyTreeRecursive(pNode->node1, phWndPreserve, cPreserve);
    DockHostRuntime_DestroyTreeRecursive(pNode->node2, phWndPreserve, cPreserve);

    DockData* pDockData = (DockData*)pNode->data;
    if (pDockData && pDockData->hWnd && IsWindow(pDockData->hWnd) &&
        !DockHostRuntime_ShouldPreserveHwnd(pDockData->hWnd, phWndPreserve, cPreserve))
    {
        DestroyWindow(pDockData->hWnd);
    }

    free(pNode->data);
    free(pNode);
}

BOOL DockHostWindow_GetHostContentRect(DockHostWindow* pDockHostWindow, RECT* pRect)
{
    if (!pDockHostWindow || !pRect)
    {
        return FALSE;
    }

    HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    GetClientRect(hWnd, pRect);
    TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
    if (pRoot && pRoot->data)
    {
        DockData* pRootData = (DockData*)pRoot->data;
        RECT rcRootClient = { 0 };
        if (DockData_GetClientRect(pRootData, &rcRootClient))
        {
            *pRect = rcRootClient;
        }
    }

    return Win32_Rect_GetWidth(pRect) > 0 && Win32_Rect_GetHeight(pRect) > 0;
}

void DockHostWindow_DestroyNodeTree(TreeNode* pRootNode, const HWND* phWndPreserve, int cPreserve)
{
    DockHostRuntime_DestroyTreeRecursive(pRootNode, phWndPreserve, cPreserve);
}

void DockHostWindow_ClearLayout(DockHostWindow* pDockHostWindow, const HWND* phWndPreserve, int cPreserve)
{
    if (!pDockHostWindow)
    {
        return;
    }

    DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
    DockHostInput_ClearCaptionState(pDockHostWindow);
    DockHostWindow_ClearAutoHideCaptionState(pDockHostWindow);
    DockHostWindow_DestroyNodeTree(pDockHostWindow->pRoot_, phWndPreserve, cPreserve);
    pDockHostWindow->pRoot_ = NULL;
    if (pDockHostWindow->m_pDockInspectorDialog)
    {
        DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, NULL);
    }
    Window_Invalidate((Window*)pDockHostWindow);
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
        DockHostRuntime_AssignPersistentNameForHWND(pDockData, hWnd);
    }
}

void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window)
{
    HWND hWnd = Window_GetHWND(window);
    DockData_PinHWND(pDockHostWindow, pDockData, hWnd);
}

void DockHostWindow_RefreshTheme(DockHostWindow* pDockHostWindow)
{
    PanitentThemeColors colors = { 0 };
    HWND hWnd = NULL;

    if (!pDockHostWindow)
    {
        return;
    }

    PanitentTheme_GetColors(&colors);
    if (pDockHostWindow->hCaptionBrush_)
    {
        DeleteObject(pDockHostWindow->hCaptionBrush_);
    }
    pDockHostWindow->hCaptionBrush_ = CreateSolidBrush(colors.accent);

    hWnd = Window_GetHWND((Window*)pDockHostWindow);
    if (hWnd && IsWindow(hWnd))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (pDockHostWindow->hWndAutoHideOverlayHost && IsWindow(pDockHostWindow->hWndAutoHideOverlayHost))
    {
        RedrawWindow(
            pDockHostWindow->hWndAutoHideOverlayHost,
            NULL,
            NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
}

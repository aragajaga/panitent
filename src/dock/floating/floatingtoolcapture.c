#include "precomp.h"

#include "floatingtoolcapture.h"

#include "dockmodel.h"
#include "dockviewcatalog.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "win32/window.h"
#include "win32/windowmap.h"

typedef struct FloatingToolHostEnumContext
{
    FnFloatingToolHostWindowCallback pfnCallback;
    void* pUserData;
    BOOL bResult;
} FloatingToolHostEnumContext;

static BOOL FloatingToolHost_IsClassName(HWND hWnd, PCWSTR pszClassName)
{
    WCHAR szClassName[64] = L"";
    if (!hWnd || !IsWindow(hWnd) || !pszClassName)
    {
        return FALSE;
    }

    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return wcscmp(szClassName, pszClassName) == 0;
}

BOOL FloatingToolHost_IsPinnedFloatingWindow(HWND hWnd, FloatingWindowContainer** ppFloatingWindowContainer)
{
    DWORD processId = 0;
    FloatingWindowContainer* pFloatingWindowContainer = NULL;

    if (ppFloatingWindowContainer)
    {
        *ppFloatingWindowContainer = NULL;
    }

    if (!hWnd || !IsWindow(hWnd))
    {
        return FALSE;
    }

    GetWindowThreadProcessId(hWnd, &processId);
    if (processId != GetCurrentProcessId())
    {
        return FALSE;
    }

    if (!FloatingToolHost_IsClassName(hWnd, L"__FloatingWindowContainer"))
    {
        return FALSE;
    }

    pFloatingWindowContainer = (FloatingWindowContainer*)WindowMap_Get(hWnd);
    if (!pFloatingWindowContainer || pFloatingWindowContainer->nDockPolicy != FLOAT_DOCK_POLICY_PANEL)
    {
        return FALSE;
    }

    if (ppFloatingWindowContainer)
    {
        *ppFloatingWindowContainer = pFloatingWindowContainer;
    }
    return TRUE;
}

static BOOL CALLBACK FloatingToolHost_EnumPinnedWindowsProc(HWND hWnd, LPARAM lParam)
{
    FloatingToolHostEnumContext* pContext = (FloatingToolHostEnumContext*)lParam;
    FloatingWindowContainer* pFloatingWindowContainer = NULL;
    if (!pContext)
    {
        return TRUE;
    }

    if (!FloatingToolHost_IsPinnedFloatingWindow(hWnd, &pFloatingWindowContainer))
    {
        return TRUE;
    }

    if (!pContext->pfnCallback)
    {
        return TRUE;
    }

    if (!pContext->pfnCallback(hWnd, pFloatingWindowContainer, pContext->pUserData))
    {
        pContext->bResult = FALSE;
        return FALSE;
    }

    return TRUE;
}

BOOL FloatingToolHost_ForEachPinnedWindow(FnFloatingToolHostWindowCallback pfnCallback, void* pUserData)
{
    FloatingToolHostEnumContext context = { 0 };
    context.pfnCallback = pfnCallback;
    context.pUserData = pUserData;
    context.bResult = TRUE;
    EnumWindows(FloatingToolHost_EnumPinnedWindowsProc, (LPARAM)&context);
    return context.bResult;
}

static BOOL FloatingToolHost_OnDestroyPinnedWindow(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    void* pUserData)
{
    UNREFERENCED_PARAMETER(pFloatingWindowContainer);
    UNREFERENCED_PARAMETER(pUserData);
    DestroyWindow(hWndFloating);
    return TRUE;
}

void FloatingToolHost_DestroyExistingPinnedWindows(void)
{
    FloatingToolHost_ForEachPinnedWindow(FloatingToolHost_OnDestroyPinnedWindow, NULL);
}

BOOL FloatingToolHost_CapturePinnedWindowState(
    HWND hWndFloating,
    FloatingWindowContainer* pFloatingWindowContainer,
    DockFloatingLayoutEntry* pEntry)
{
    if (!hWndFloating || !IsWindow(hWndFloating) || !pFloatingWindowContainer || !pEntry)
    {
        return FALSE;
    }

    HWND hWndChild = pFloatingWindowContainer->hWndChild;
    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FALSE;
    }

    FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
    if (nChildKind != FLOAT_DOCK_CHILD_TOOL_PANEL &&
        nChildKind != FLOAT_DOCK_CHILD_TOOL_HOST)
    {
        return FALSE;
    }

    memset(pEntry, 0, sizeof(*pEntry));
    GetWindowRect(hWndFloating, &pEntry->rcWindow);
    pEntry->iDockSizeHint = pFloatingWindowContainer->iDockSizeHint;
    pEntry->nChildKind = nChildKind;

    if (nChildKind == FLOAT_DOCK_CHILD_TOOL_PANEL)
    {
        WCHAR szTitle[128] = L"";
        WCHAR szClassName[64] = L"";
        GetWindowTextW(hWndChild, szTitle, ARRAYSIZE(szTitle));
        GetClassNameW(hWndChild, szClassName, ARRAYSIZE(szClassName));

        pEntry->nViewId = PanitentDockViewCatalog_FindForWindow(szClassName, szTitle);
        return pEntry->nViewId != PNT_DOCK_VIEW_NONE;
    }

    DockHostWindow* pFloatingDockHost = (DockHostWindow*)WindowMap_Get(hWndChild);
    pEntry->pLayoutModel = pFloatingDockHost ? DockModel_CaptureHostLayout(pFloatingDockHost) : NULL;
    return pEntry->pLayoutModel != NULL;
}

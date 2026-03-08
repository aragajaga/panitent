#include "precomp.h"

#include "win32/window.h"
#include "win32/windowmap.h"
#include "floatingchildhost.h"
#include "floatingwindowcontainer.h"
#include "dockgroup.h"
#include "dockhost.h"
#include "dockshell.h"
#include "workspacecontainer.h"
#include "panitentapp.h"

typedef struct WorkspaceHwndCollectContext WorkspaceHwndCollectContext;
struct WorkspaceHwndCollectContext
{
    HWND* pWorkspaceHwnds;
    int cWorkspaceHwnds;
    int nWorkspaceCount;
};

static BOOL FloatingChildHost_IsClassName(HWND hWnd, PCWSTR pszClassName)
{
    if (!hWnd || !IsWindow(hWnd) || !pszClassName)
    {
        return FALSE;
    }

    WCHAR szClassName[64] = L"";
    GetClassNameW(hWnd, szClassName, ARRAYSIZE(szClassName));
    return wcscmp(szClassName, pszClassName) == 0;
}

static BOOL CALLBACK FloatingChildHost_EnumChildWorkspaceHwndProc(HWND hWnd, LPARAM lParam)
{
    WorkspaceHwndCollectContext* pContext = (WorkspaceHwndCollectContext*)lParam;
    if (!pContext)
    {
        return TRUE;
    }

    if (FloatingChildHost_IsClassName(hWnd, L"__WorkspaceContainer"))
    {
        if (pContext->nWorkspaceCount < pContext->cWorkspaceHwnds && pContext->pWorkspaceHwnds)
        {
            pContext->pWorkspaceHwnds[pContext->nWorkspaceCount] = hWnd;
        }
        pContext->nWorkspaceCount++;
    }

    return TRUE;
}

static int FloatingChildHost_CollectWorkspaceHwnds(HWND hWndRoot, HWND* pWorkspaceHwnds, int cWorkspaceHwnds)
{
    WorkspaceHwndCollectContext context = { 0 };
    if (!hWndRoot || !IsWindow(hWndRoot) || !pWorkspaceHwnds || cWorkspaceHwnds <= 0)
    {
        return 0;
    }

    context.pWorkspaceHwnds = pWorkspaceHwnds;
    context.cWorkspaceHwnds = cWorkspaceHwnds;
    context.nWorkspaceCount = 0;
    EnumChildWindows(hWndRoot, FloatingChildHost_EnumChildWorkspaceHwndProc, (LPARAM)&context);
    return min(context.nWorkspaceCount, cWorkspaceHwnds);
}

FloatingDockChildHostKind FloatingChildHost_GetKind(HWND hWndChild)
{
    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FLOAT_DOCK_CHILD_NONE;
    }

    if (FloatingChildHost_IsClassName(hWndChild, L"__WorkspaceContainer"))
    {
        return FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE;
    }

    if (!FloatingChildHost_IsClassName(hWndChild, L"__DockHostWindow"))
    {
        return FLOAT_DOCK_CHILD_TOOL_PANEL;
    }

    Window* pWindow = WindowMap_Get(hWndChild);
    DockHostWindow* pDockHostWindow = (DockHostWindow*)pWindow;
    TreeNode* pRoot = pDockHostWindow ? DockHostWindow_GetRoot(pDockHostWindow) : NULL;
    if (DockGroup_NodeContainsPaneKind(pRoot, DOCK_PANE_DOCUMENT))
    {
        return FLOAT_DOCK_CHILD_DOCUMENT_HOST;
    }

    return FLOAT_DOCK_CHILD_TOOL_HOST;
}

int FloatingChildHost_CollectDocumentWorkspaceHwnds(HWND hWndChild, HWND* pWorkspaceHwnds, int cWorkspaceHwnds)
{
    FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
    if (!pWorkspaceHwnds || cWorkspaceHwnds <= 0)
    {
        return 0;
    }

    if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE)
    {
        pWorkspaceHwnds[0] = hWndChild;
        return 1;
    }

    if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_HOST)
    {
        return FloatingChildHost_CollectWorkspaceHwnds(hWndChild, pWorkspaceHwnds, cWorkspaceHwnds);
    }

    return 0;
}

BOOL FloatingChildHost_GetDocumentSourceContext(HWND hWndChild, POINT ptScreen, WorkspaceContainer** ppWorkspaceSource, int* pnSourceDocumentCount)
{
    if (ppWorkspaceSource)
    {
        *ppWorkspaceSource = NULL;
    }
    if (pnSourceDocumentCount)
    {
        *pnSourceDocumentCount = 0;
    }

    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FALSE;
    }

    FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
    if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE)
    {
        Window* pWindow = WindowMap_Get(hWndChild);
        WorkspaceContainer* pWorkspaceSource = (WorkspaceContainer*)pWindow;
        if (!pWorkspaceSource)
        {
            return FALSE;
        }

        if (ppWorkspaceSource)
        {
            *ppWorkspaceSource = pWorkspaceSource;
        }
        if (pnSourceDocumentCount)
        {
            *pnSourceDocumentCount = WorkspaceContainer_GetViewportCount(pWorkspaceSource);
        }
        return TRUE;
    }

    if (nChildKind != FLOAT_DOCK_CHILD_DOCUMENT_HOST)
    {
        return FALSE;
    }

    HWND hWndWorkspaceList[64] = { 0 };
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(hWndChild, hWndWorkspaceList, ARRAYSIZE(hWndWorkspaceList));
    if (nWorkspaceCount <= 0)
    {
        return FALSE;
    }

    int nPreferredIndex = -1;
    int nDocumentCount = 0;
    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        Window* pWindow = WindowMap_Get(hWndWorkspaceList[i]);
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)pWindow;
        if (!pWorkspace)
        {
            continue;
        }

        nDocumentCount += WorkspaceContainer_GetViewportCount(pWorkspace);
        if (nPreferredIndex < 0)
        {
            nPreferredIndex = i;
        }

        RECT rcWorkspace = { 0 };
        GetWindowRect(hWndWorkspaceList[i], &rcWorkspace);
        if (PtInRect(&rcWorkspace, ptScreen))
        {
            nPreferredIndex = i;
            break;
        }
    }

    if (nPreferredIndex < 0)
    {
        return FALSE;
    }

    Window* pPreferredWindow = WindowMap_Get(hWndWorkspaceList[nPreferredIndex]);
    if (!pPreferredWindow)
    {
        return FALSE;
    }

    if (ppWorkspaceSource)
    {
        *ppWorkspaceSource = (WorkspaceContainer*)pPreferredWindow;
    }
    if (pnSourceDocumentCount)
    {
        *pnSourceDocumentCount = nDocumentCount;
    }

    return TRUE;
}

BOOL FloatingChildHost_MoveDocumentsToWorkspace(HWND hWndChild, WorkspaceContainer* pWorkspaceTarget)
{
    if (!hWndChild || !IsWindow(hWndChild) || !pWorkspaceTarget)
    {
        return FALSE;
    }

    FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
    if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE)
    {
        Window* pWindow = WindowMap_Get(hWndChild);
        if (!pWindow)
        {
            return FALSE;
        }

        WorkspaceContainer_MoveAllViewportsTo((WorkspaceContainer*)pWindow, pWorkspaceTarget);
        return TRUE;
    }

    if (nChildKind != FLOAT_DOCK_CHILD_DOCUMENT_HOST)
    {
        return FALSE;
    }

    HWND hWndWorkspaceList[64] = { 0 };
    int nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(hWndChild, hWndWorkspaceList, ARRAYSIZE(hWndWorkspaceList));
    if (nWorkspaceCount <= 0)
    {
        return FALSE;
    }

    BOOL bMoved = FALSE;
    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        Window* pWindow = WindowMap_Get(hWndWorkspaceList[i]);
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)pWindow;
        if (!pWorkspace || pWorkspace == pWorkspaceTarget)
        {
            continue;
        }

        if (WorkspaceContainer_GetViewportCount(pWorkspace) <= 0)
        {
            continue;
        }

        WorkspaceContainer_MoveAllViewportsTo(pWorkspace, pWorkspaceTarget);
        bMoved = TRUE;
    }

    return bMoved;
}

BOOL FloatingChildHost_EnsureWorkspaceChildForSideDock(HWND hWndFloating, HWND* phWndChild)
{
    HWND hWndChild;
    WorkspaceContainer* pMergedWorkspace;
    HWND hWndMergedWorkspace;
    HWND hWndWorkspaceList[64] = { 0 };
    int nWorkspaceCount;
    BOOL bMoved = FALSE;

    if (!hWndFloating || !IsWindow(hWndFloating) || !phWndChild)
    {
        return FALSE;
    }

    hWndChild = *phWndChild;
    if (!hWndChild || !IsWindow(hWndChild))
    {
        return FALSE;
    }

    FloatingDockChildHostKind nChildKind = FloatingChildHost_GetKind(hWndChild);
    if (nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE)
    {
        return TRUE;
    }

    if (!FloatingDockPolicy_RequiresWorkspaceMergeForSideDock(nChildKind))
    {
        return FALSE;
    }

    pMergedWorkspace = WorkspaceContainer_Create();
    if (!pMergedWorkspace)
    {
        return FALSE;
    }

    hWndMergedWorkspace = Window_CreateWindow((Window*)pMergedWorkspace, hWndFloating);
    if (!hWndMergedWorkspace || !IsWindow(hWndMergedWorkspace))
    {
        free(pMergedWorkspace);
        return FALSE;
    }

    nWorkspaceCount = FloatingChildHost_CollectDocumentWorkspaceHwnds(hWndChild, hWndWorkspaceList, ARRAYSIZE(hWndWorkspaceList));
    if (nWorkspaceCount <= 0)
    {
        DestroyWindow(hWndMergedWorkspace);
        return FALSE;
    }

    for (int i = 0; i < nWorkspaceCount; ++i)
    {
        Window* pWindow = WindowMap_Get(hWndWorkspaceList[i]);
        WorkspaceContainer* pWorkspace = (WorkspaceContainer*)pWindow;
        if (!pWorkspace || WorkspaceContainer_GetViewportCount(pWorkspace) <= 0)
        {
            continue;
        }

        WorkspaceContainer_MoveAllViewportsTo(pWorkspace, pMergedWorkspace);
        bMoved = TRUE;
    }

    if (!bMoved)
    {
        DestroyWindow(hWndMergedWorkspace);
        return FALSE;
    }

    DestroyWindow(hWndChild);
    *phWndChild = hWndMergedWorkspace;

    RECT rcClient = { 0 };
    GetClientRect(hWndFloating, &rcClient);
    SetWindowPos(
        hWndMergedWorkspace,
        HWND_TOP,
        0,
        0,
        max(0, Win32_Rect_GetWidth(&rcClient)),
        max(0, Win32_Rect_GetHeight(&rcClient)),
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED);

    return TRUE;
}

DockHostWindow* FloatingChildHost_EnsureTargetWorkspaceDockHost(HWND hWndTargetWorkspace)
{
    HWND hWndTargetParent;
    Window* pFloatingWindow;
    FloatingWindowContainer* pFloatingWindowContainer;
    DockHostWindow* pLocalDockHostWindow;
    HWND hWndLocalDockHostWindow;
    TreeNode* pRootNode;
    TreeNode* pWorkspaceNode;
    DockData* pRootData;
    Window* pTargetWorkspaceWindow;

    if (!hWndTargetWorkspace || !IsWindow(hWndTargetWorkspace))
    {
        return NULL;
    }

    hWndTargetParent = GetParent(hWndTargetWorkspace);
    if (!hWndTargetParent || !IsWindow(hWndTargetParent))
    {
        return NULL;
    }

    if (FloatingChildHost_IsClassName(hWndTargetParent, L"__DockHostWindow"))
    {
        return (DockHostWindow*)WindowMap_Get(hWndTargetParent);
    }

    if (!FloatingChildHost_IsClassName(hWndTargetParent, L"__FloatingWindowContainer"))
    {
        return NULL;
    }

    pFloatingWindow = WindowMap_Get(hWndTargetParent);
    pFloatingWindowContainer = (FloatingWindowContainer*)pFloatingWindow;
    if (!pFloatingWindowContainer || !pFloatingWindowContainer->hWndChild || !IsWindow(pFloatingWindowContainer->hWndChild))
    {
        return NULL;
    }

    if (FloatingChildHost_IsClassName(pFloatingWindowContainer->hWndChild, L"__DockHostWindow"))
    {
        return (DockHostWindow*)WindowMap_Get(pFloatingWindowContainer->hWndChild);
    }

    if (pFloatingWindowContainer->hWndChild != hWndTargetWorkspace ||
        !FloatingChildHost_IsClassName(hWndTargetWorkspace, L"__WorkspaceContainer"))
    {
        return NULL;
    }

    pLocalDockHostWindow = DockHostWindow_Create(PanitentApp_Instance());
    if (!pLocalDockHostWindow)
    {
        return NULL;
    }

    hWndLocalDockHostWindow = Window_CreateWindow((Window*)pLocalDockHostWindow, hWndTargetParent);
    if (!hWndLocalDockHostWindow || !IsWindow(hWndLocalDockHostWindow))
    {
        free(pLocalDockHostWindow);
        return NULL;
    }

    pRootNode = DockShell_CreateRootNode();
    pWorkspaceNode = DockShell_CreateWorkspaceNode();
    if (!pRootNode || !pRootNode->data || !pWorkspaceNode || !pWorkspaceNode->data)
    {
        if (pRootNode)
        {
            free(pRootNode->data);
            free(pRootNode);
        }
        if (pWorkspaceNode)
        {
            free(pWorkspaceNode->data);
            free(pWorkspaceNode);
        }
        DestroyWindow(hWndLocalDockHostWindow);
        return NULL;
    }

    pRootData = (DockData*)pRootNode->data;
    GetClientRect(hWndTargetParent, &pRootData->rc);

    pTargetWorkspaceWindow = WindowMap_Get(hWndTargetWorkspace);
    if (!pTargetWorkspaceWindow)
    {
        DestroyWindow(hWndLocalDockHostWindow);
        return NULL;
    }

    DockData* pWorkspaceData = (DockData*)pWorkspaceNode->data;
    pWorkspaceData->bShowCaption = FALSE;
    DockData_PinWindow(pLocalDockHostWindow, pWorkspaceData, pTargetWorkspaceWindow);

    DockShell_BuildWorkspaceOnlyLayout(pRootNode, pWorkspaceNode);
    DockHostWindow_SetRoot(pLocalDockHostWindow, pRootNode);
    pFloatingWindowContainer->hWndChild = hWndLocalDockHostWindow;

    RECT rcFloatingClient = { 0 };
    GetClientRect(hWndTargetParent, &rcFloatingClient);
    SetWindowPos(
        hWndLocalDockHostWindow,
        HWND_TOP,
        0,
        0,
        max(0, Win32_Rect_GetWidth(&rcFloatingClient)),
        max(0, Win32_Rect_GetHeight(&rcFloatingClient)),
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED);

    return pLocalDockHostWindow;
}

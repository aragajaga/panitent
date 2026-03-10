#include "precomp.h"

#include "dockhostsetup.h"

#include "dockhostlifecycle.h"
#include "dockinspectordialog.h"
#include "panitentapp.h"
#include "win32/window.h"

void DockHostWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_VREDRAW | CS_HREDRAW;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    lpwcex->lpszClassName = L"__DockHostWindow";
}

void DockHostWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = L"__DockHostWindow";
    lpcs->lpszName = L"DockHost";
    lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN;
    lpcs->x = 0;
    lpcs->y = 0;
    lpcs->cx = 0;
    lpcs->cy = 0;
}

DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
    DockData* pDockData = (DockData*)malloc(sizeof(DockData));

    if (pDockData)
    {
        memset(pDockData, 0, sizeof(DockData));

        pDockData->dwStyle = dwStyle;
        pDockData->fGripPos = -1.0f;
        pDockData->iGripPos = iGripPos;
        pDockData->bShowCaption = bShowCaption;
        pDockData->bCollapsed = FALSE;
        pDockData->hWndActiveTab = NULL;
        pDockData->nRole = DOCK_ROLE_UNKNOWN;
        pDockData->nPaneKind = DOCK_PANE_NONE;
        pDockData->nDockSide = DKS_NONE;
        pDockData->uModelNodeId = 0;
        pDockData->nViewId = PNT_DOCK_VIEW_NONE;
    }

    return pDockData;
}

TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
    TreeNode* pTreeNode = BinaryTree_AllocEmptyNode();
    DockData* pDockData = DockData_Create(iGripPos, dwStyle, bShowCaption);

    if (!pTreeNode || !pDockData)
    {
        free(pTreeNode);
        free(pDockData);
        return NULL;
    }

    pTreeNode->data = pDockData;
    return pTreeNode;
}

void DockHostWindow_Init(DockHostWindow* pDockHostWindow, PanitentApp* pPanitentApp)
{
    Window_Init(&pDockHostWindow->base);

    pDockHostWindow->base.szClassName = L"__DockHostWindow";
    pDockHostWindow->base.OnCreate = (FnWindowOnCreate)DockHostWindow_OnCreate;
    pDockHostWindow->base.OnDestroy = (FnWindowOnDestroy)DockHostWindow_OnDestroy;
    pDockHostWindow->base.OnPaint = (FnWindowOnPaint)DockHostWindow_OnPaint;
    pDockHostWindow->base.OnSize = (FnWindowOnSize)DockHostWindow_OnSize;
    pDockHostWindow->base.OnCommand = (FnWindowOnCommand)DockHostWindow_OnCommand;

    _WindowInitHelper_SetPreRegisterRoutine((Window*)pDockHostWindow, (FnWindowPreRegister)DockHostWindow_PreRegister);
    _WindowInitHelper_SetPreCreateRoutine((Window*)pDockHostWindow, (FnWindowPreCreate)DockHostWindow_PreCreate);
    _WindowInitHelper_SetUserProcRoutine((Window*)pDockHostWindow, (FnWindowUserProc)DockHostWindow_UserProc);

    pDockHostWindow->pRoot_ = NULL;
    pDockHostWindow->fSplitDrag = FALSE;
    pDockHostWindow->pSplitNode = NULL;
    pDockHostWindow->iSplitDragStartGrip = 0;
    pDockHostWindow->fAutoHideOverlayVisible = FALSE;
    pDockHostWindow->nAutoHideOverlaySide = DKS_NONE;
    pDockHostWindow->hWndAutoHideOverlay = NULL;
    SetRectEmpty(&pDockHostWindow->rcAutoHideOverlay);
    pDockHostWindow->pCaptionHotNode = NULL;
    pDockHostWindow->pCaptionPressedNode = NULL;
    pDockHostWindow->nCaptionHotButton = DCB_NONE;
    pDockHostWindow->nCaptionPressedButton = DCB_NONE;
    pDockHostWindow->fAutoHideOverlayTrackMouse = FALSE;
    pDockHostWindow->nAutoHideOverlayHotButton = DCB_NONE;
    pDockHostWindow->nAutoHideOverlayPressedButton = DCB_NONE;
    pDockHostWindow->m_pDockInspectorDialog = DockInspectorDialog_Create();
    UNREFERENCED_PARAMETER(pPanitentApp);
}

DockHostWindow* DockHostWindow_Create(PanitentApp* pPanitentApp)
{
    DockHostWindow* pDockHostWindow = (DockHostWindow*)malloc(sizeof(DockHostWindow));

    if (pDockHostWindow)
    {
        memset(pDockHostWindow, 0, sizeof(DockHostWindow));
        DockHostWindow_Init(pDockHostWindow, pPanitentApp);
    }

    return pDockHostWindow;
}

TreeNode* DockHostWindow_SetRoot(DockHostWindow* pDockHostWindow, TreeNode* pNewRoot)
{
    TreeNode* pOldRoot = pDockHostWindow->pRoot_;
    pDockHostWindow->pRoot_ = pNewRoot;
    return pOldRoot;
}

TreeNode* DockHostWindow_GetRoot(DockHostWindow* pDockHostWindow)
{
    return pDockHostWindow->pRoot_;
}

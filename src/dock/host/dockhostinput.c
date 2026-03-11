#include "precomp.h"

#include "dockhostinput.h"

#include "win32/dialog.h"
#include "win32/window.h"
#include "dockhostautohide.h"
#include "dockhostcaption.h"
#include "dockhostdrag.h"
#include "dockhostlayout.h"
#include "dockhostpaint.h"
#include "dockhostpanelmenu.h"
#include "dockhosttree.h"
#include "dockinspectordialog.h"
#include "docklayout.h"
#include "dockpolicy.h"
#include "resource.h"
#include "util/tree.h"
#include "win32/util.h"

#define DHT_UNKNOWN 0
#define DHT_CAPTION 1
#define DHT_CLOSEBTN 2
#define DHT_PINBTN 3
#define DHT_MOREBTN 4

#define DOCK_MIN_PANE_SIZE 96
#define IDM_DOCKINSPECTOR 101

static int DockHostInput_HitTypeToCaptionButton(int nHitType)
{
    switch (nHitType)
    {
    case DHT_CLOSEBTN:
        return DCB_CLOSE;
    case DHT_PINBTN:
        return DCB_PIN;
    case DHT_MOREBTN:
        return DCB_MORE;
    default:
        return DCB_NONE;
    }
}

static void DockHostInput_SetCaptionHotButton(DockHostWindow* pDockHostWindow, TreeNode* pNode, int nButton)
{
    if (!pDockHostWindow)
    {
        return;
    }

    if (nButton == DCB_NONE)
    {
        pNode = NULL;
    }

    if (pDockHostWindow->pCaptionHotNode == pNode &&
        pDockHostWindow->nCaptionHotButton == nButton)
    {
        return;
    }

    pDockHostWindow->pCaptionHotNode = pNode;
    pDockHostWindow->nCaptionHotButton = nButton;
    Window_Invalidate((Window*)pDockHostWindow);
}

static void DockHostInput_SetCaptionPressedButton(DockHostWindow* pDockHostWindow, TreeNode* pNode, int nButton)
{
    if (!pDockHostWindow)
    {
        return;
    }

    if (nButton == DCB_NONE)
    {
        pNode = NULL;
    }

    if (pDockHostWindow->pCaptionPressedNode == pNode &&
        pDockHostWindow->nCaptionPressedButton == nButton)
    {
        return;
    }

    pDockHostWindow->pCaptionPressedNode = pNode;
    pDockHostWindow->nCaptionPressedButton = nButton;
    Window_Invalidate((Window*)pDockHostWindow);
}

void DockHostInput_ClearCaptionState(DockHostWindow* pDockHostWindow)
{
    if (!pDockHostWindow)
    {
        return;
    }

    BOOL bChanged =
        (pDockHostWindow->pCaptionHotNode != NULL) ||
        (pDockHostWindow->nCaptionHotButton != DCB_NONE) ||
        (pDockHostWindow->pCaptionPressedNode != NULL) ||
        (pDockHostWindow->nCaptionPressedButton != DCB_NONE);

    pDockHostWindow->pCaptionHotNode = NULL;
    pDockHostWindow->nCaptionHotButton = DCB_NONE;
    pDockHostWindow->pCaptionPressedNode = NULL;
    pDockHostWindow->nCaptionPressedButton = DCB_NONE;

    if (bChanged)
    {
        Window_Invalidate((Window*)pDockHostWindow);
    }
}

static int DockHostInput_HitTest(DockHostWindow* pDockHostWindow, TreeNode** ppTreeNode, int x, int y)
{
    if (!pDockHostWindow->pRoot_)
    {
        return 0;
    }

    TreeTraversalRLOT traversal = { 0 };
    TreeTraversalRLOT_Init(&traversal, pDockHostWindow->pRoot_);

    TreeNode* pCurrentNode = NULL;
    TreeNode* pHittedNode = NULL;
    while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
    {
        DockData* pDockData = (DockData*)pCurrentNode->data;
        POINT pt = { x, y };
        if (PtInRect(&pDockData->rc, pt))
        {
            pHittedNode = pCurrentNode;
            break;
        }
    }
    TreeTraversalRLOT_Destroy(&traversal);

    *ppTreeNode = pHittedNode;
    if (pHittedNode && pHittedNode->data)
    {
        DockData* pDockDataHit = (DockData*)pHittedNode->data;
        if (Dock_CloseButtonHitTest(pDockDataHit, x, y) && DockPolicy_CanClosePanel(pDockDataHit->nRole, pDockDataHit->lpszName))
        {
            return DHT_CLOSEBTN;
        }
        if (Dock_PinButtonHitTest(pDockDataHit, x, y) && DockPolicy_CanPinPanel(pDockDataHit->nRole, pDockDataHit->lpszName))
        {
            return DHT_PINBTN;
        }
        if (Dock_MoreButtonHitTest(pDockDataHit, x, y))
        {
            return DHT_MOREBTN;
        }
        if (Dock_CaptionHitTest(pDockDataHit, x, y) && DockPolicy_CanUndockPanel(pDockDataHit->nRole, pDockDataHit->lpszName))
        {
            return DHT_CAPTION;
        }
    }

    return DHT_UNKNOWN;
}

static BOOL DockHostInput_NodeUsesProportionalGrip(TreeNode* pNode)
{
    if (!pNode || !pNode->data)
    {
        return FALSE;
    }

    DockData* pDockData = (DockData*)pNode->data;
    return DockNodeRole_UsesProportionalGrip(pDockData->nRole, pDockData->lpszName);
}

static TreeNode* DockHostInput_HitTestSplitGrip(DockHostWindow* pDockHostWindow, int x, int y)
{
    if (!pDockHostWindow || !pDockHostWindow->pRoot_)
    {
        return NULL;
    }

    POINT pt = { x, y };
    TreeTraversalRLOT traversal = { 0 };
    TreeTraversalRLOT_Init(&traversal, pDockHostWindow->pRoot_);

    TreeNode* pResult = NULL;
    TreeNode* pCurrentNode = NULL;
    while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
    {
        RECT rcSplit = { 0 };
        if (!DockHostLayout_GetSplitRect(pCurrentNode, &rcSplit, 4))
        {
            continue;
        }

        InflateRect(&rcSplit, 3, 3);
        if (PtInRect(&rcSplit, pt))
        {
            pResult = pCurrentNode;
            break;
        }
    }

    TreeTraversalRLOT_Destroy(&traversal);
    return pResult;
}

static void DockHostInput_BeginSplitDrag(DockHostWindow* pDockHostWindow, TreeNode* pSplitNode, int x, int y)
{
    if (!pDockHostWindow || !pSplitNode || !pSplitNode->data)
    {
        return;
    }

    pDockHostWindow->fSplitDrag = TRUE;
    pDockHostWindow->pSplitNode = pSplitNode;
    pDockHostWindow->ptSplitDragStart.x = x;
    pDockHostWindow->ptSplitDragStart.y = y;
    pDockHostWindow->iSplitDragStartGrip = ((DockData*)pSplitNode->data)->iGripPos;
    SetCapture(Window_GetHWND((Window*)pDockHostWindow));
}

static void DockHostInput_UpdateSplitDrag(DockHostWindow* pDockHostWindow, int x, int y)
{
    if (!pDockHostWindow || !pDockHostWindow->fSplitDrag || !pDockHostWindow->pSplitNode || !pDockHostWindow->pSplitNode->data)
    {
        return;
    }

    DockData* pDockData = (DockData*)pDockHostWindow->pSplitNode->data;
    if (pDockData->dwStyle & DGP_RELATIVE)
    {
        return;
    }

    RECT rcClient = { 0 };
    if (!DockData_GetClientRect(pDockData, &rcClient))
    {
        return;
    }

    BOOL bVertical = DockHostLayout_IsSplitVertical(pDockHostWindow->pSplitNode);
    int iSpan = bVertical ? Win32_Rect_GetHeight(&rcClient) : Win32_Rect_GetWidth(&rcClient);
    if (iSpan <= 0)
    {
        return;
    }

    int iDelta = bVertical ? (y - pDockHostWindow->ptSplitDragStart.y) : (x - pDockHostWindow->ptSplitDragStart.x);
    int iNextGrip = DockLayout_AdjustSplitGripFromDelta(pDockData->dwStyle, pDockHostWindow->iSplitDragStartGrip, iDelta, iSpan, DOCK_MIN_PANE_SIZE);
    if (iNextGrip == pDockData->iGripPos)
    {
        return;
    }

    pDockData->iGripPos = (short)iNextGrip;
    if (DockHostInput_NodeUsesProportionalGrip(pDockHostWindow->pSplitNode))
    {
        pDockData->fGripPos = DockLayout_GetGripRatio(iSpan, iNextGrip, DOCK_MIN_PANE_SIZE);
    }
    DockHostWindow_Rearrange(pDockHostWindow);
}

static void DockHostInput_EndSplitDrag(DockHostWindow* pDockHostWindow)
{
    if (!pDockHostWindow || !pDockHostWindow->fSplitDrag)
    {
        return;
    }

    pDockHostWindow->fSplitDrag = FALSE;
    pDockHostWindow->pSplitNode = NULL;
    pDockHostWindow->iSplitDragStartGrip = 0;
    ReleaseCapture();
}

void DockHostInput_InvokeInspectorDialog(DockHostWindow* pDockHostWindow)
{
    INT_PTR hDialog = Dialog_CreateWindow((Dialog*)pDockHostWindow->m_pDockInspectorDialog, IDD_DOCKINSPECTOR, Window_GetHWND((Window*)pDockHostWindow), FALSE);
    HWND hWndDialog = (HWND)hDialog;
    if (hWndDialog && IsWindow(hWndDialog))
    {
        ShowWindow(hWndDialog, SW_SHOW);
    }
}

void DockHostInput_OnMouseLeave(DockHostWindow* pDockHostWindow)
{
    DockHostInput_SetCaptionHotButton(pDockHostWindow, NULL, DCB_NONE);
}

void DockHostInput_OnCaptureChanged(DockHostWindow* pDockHostWindow)
{
    if (!pDockHostWindow)
    {
        return;
    }

    if (pDockHostWindow->fSplitDrag)
    {
        pDockHostWindow->fSplitDrag = FALSE;
        pDockHostWindow->pSplitNode = NULL;
        pDockHostWindow->iSplitDragStartGrip = 0;
    }

    DockHostInput_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
}

void DockHostInput_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags, int iZoneTabGutter)
{
    UNREFERENCED_PARAMETER(keyFlags);

    TRACKMOUSEEVENT tme = { 0 };
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = Window_GetHWND((Window*)pDockHostWindow);
    TrackMouseEvent(&tme);

    if (pDockHostWindow->fSplitDrag)
    {
        DockHostInput_ClearCaptionState(pDockHostWindow);
        DockHostInput_UpdateSplitDrag(pDockHostWindow, x, y);
        return;
    }

    if (pDockHostWindow->fCaptionDrag)
    {
        DockHostInput_ClearCaptionState(pDockHostWindow);
        int distance = (int)roundf(sqrtf(
            powf(pDockHostWindow->ptDragPos_.x - x, 2.0f) +
            powf(pDockHostWindow->ptDragPos_.y - y, 2.0f)));
        DockHostDrag_UpdateOverlayVisual(pDockHostWindow, distance);

        if (distance >= 32)
        {
            pDockHostWindow->fCaptionDrag = FALSE;
            DockHostDrag_UndockToFloating(pDockHostWindow, pDockHostWindow->m_pSubjectNode, x, y);
        }
        return;
    }

    TreeNode* pHitNode = NULL;
    int nHitType = DockHostInput_HitTest(pDockHostWindow, &pHitNode, x, y);
    DockHostInput_SetCaptionHotButton(pDockHostWindow, pHitNode, DockHostInput_HitTypeToCaptionButton(nHitType));

    TreeNode* pSplitNode = DockHostInput_HitTestSplitGrip(pDockHostWindow, x, y);
    if (pSplitNode && pSplitNode->data)
    {
        SetCursor(LoadCursor(NULL, DockHostLayout_IsSplitVertical(pSplitNode) ? IDC_SIZENS : IDC_SIZEWE));
    }

    UNREFERENCED_PARAMETER(iZoneTabGutter);
}

void DockHostInput_OnLButtonDown(DockHostWindow* pDockHostWindow, BOOL fDoubleClick, int x, int y, UINT keyFlags, int iZoneTabGutter)
{
    UNREFERENCED_PARAMETER(fDoubleClick);
    UNREFERENCED_PARAMETER(keyFlags);

    HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);
    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);

    if (pDockHostWindow->fAutoHideOverlayVisible)
    {
        POINT pt = { x, y };
        if (PtInRect(&pDockHostWindow->rcAutoHideOverlay, pt))
        {
            return;
        }
    }

    HWND hWndZoneTab = NULL;
    int zoneSide = DockHostPaint_HitTestZoneTab(pDockHostWindow, x, y, &hWndZoneTab, iZoneTabGutter);
    if (zoneSide != DKS_NONE)
    {
        if (pDockHostWindow->fAutoHideOverlayVisible &&
            pDockHostWindow->nAutoHideOverlaySide == zoneSide &&
            pDockHostWindow->hWndAutoHideOverlay == hWndZoneTab)
        {
            DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
        }
        else {
            DockHostWindow_ShowAutoHideOverlay(pDockHostWindow, zoneSide, hWndZoneTab);
        }
        return;
    }

    if (pDockHostWindow->fAutoHideOverlayVisible)
    {
        POINT pt = { x, y };
        if (!PtInRect(&pDockHostWindow->rcAutoHideOverlay, pt))
        {
            DockHostWindow_HideAutoHideOverlay(pDockHostWindow);
            Window_Invalidate((Window*)pDockHostWindow);
        }
    }

    TreeNode* pSplitNode = DockHostInput_HitTestSplitGrip(pDockHostWindow, x, y);
    if (pSplitNode)
    {
        DockHostInput_BeginSplitDrag(pDockHostWindow, pSplitNode, x, y);
        return;
    }

    TreeNode* pTreeNode = NULL;
    int htType = DockHostInput_HitTest(pDockHostWindow, &pTreeNode, x, y);
    int nHitButton = DockHostInput_HitTypeToCaptionButton(htType);

    switch (htType)
    {
    case DHT_CLOSEBTN:
    case DHT_PINBTN:
    case DHT_MOREBTN:
        DockHostInput_SetCaptionHotButton(pDockHostWindow, pTreeNode, nHitButton);
        DockHostInput_SetCaptionPressedButton(pDockHostWindow, pTreeNode, nHitButton);
        SetCapture(hWnd);
        return;

    case DHT_CAPTION:
        DockHostInput_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
        pDockHostWindow->fCaptionDrag = TRUE;
        pDockHostWindow->m_pSubjectNode = pTreeNode;
        SetCapture(hWnd);
        pDockHostWindow->ptDragPos_.x = x - (rcClient.left + 2);
        pDockHostWindow->ptDragPos_.y = y - (rcClient.top + 2);
        pDockHostWindow->fDrag_ = TRUE;
        DockHostDrag_StartDrag(pDockHostWindow, pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y);
        return;

    default:
        DockHostInput_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
        return;
    }
}

void DockHostInput_OnLButtonUp(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(keyFlags);

    HWND hWndDockHost = Window_GetHWND((Window*)pDockHostWindow);
    if (pDockHostWindow->fSplitDrag)
    {
        DockHostInput_EndSplitDrag(pDockHostWindow);
        return;
    }

    if (pDockHostWindow->nCaptionPressedButton != DCB_NONE)
    {
        TreeNode* pHitNode = NULL;
        int htType = DockHostInput_HitTest(pDockHostWindow, &pHitNode, x, y);
        int nHitButton = DockHostInput_HitTypeToCaptionButton(htType);

        TreeNode* pPressedNode = pDockHostWindow->pCaptionPressedNode;
        int nPressedButton = pDockHostWindow->nCaptionPressedButton;
        DockHostInput_SetCaptionPressedButton(pDockHostWindow, NULL, DCB_NONE);
        DockHostInput_SetCaptionHotButton(pDockHostWindow, pHitNode, nHitButton);

        if (GetCapture() == hWndDockHost)
        {
            ReleaseCapture();
        }

        if (pHitNode != pPressedNode || nHitButton != nPressedButton)
        {
            pDockHostWindow->fDrag_ = FALSE;
            DockHostDrag_DestroyOverlay();
            return;
        }

        switch (nPressedButton)
        {
        case DCB_CLOSE:
            DockHostWindow_DestroyInclusive(pDockHostWindow, pPressedNode);
            Window_Invalidate((Window*)pDockHostWindow);
            return;
        case DCB_PIN:
            DockHostPanelMenu_TogglePanelPinned(pDockHostWindow, pPressedNode);
            return;
        case DCB_MORE:
        {
            POINT ptScreen = { x, y };
            ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptScreen);
            DockHostPanelMenu_Show(pDockHostWindow, pPressedNode, ptScreen);
            return;
        }
        }
    }

    TreeNode* pTreeNode = NULL;
    int htType = DockHostInput_HitTest(pDockHostWindow, &pTreeNode, x, y);
    switch (htType)
    {
    case DHT_CLOSEBTN:
        DockHostWindow_DestroyInclusive(pDockHostWindow, pTreeNode);
        Window_Invalidate((Window*)pDockHostWindow);
        break;
    case DHT_PINBTN:
        DockHostPanelMenu_TogglePanelPinned(pDockHostWindow, pTreeNode);
        break;
    case DHT_MOREBTN:
    {
        POINT ptScreen = { x, y };
        ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptScreen);
        DockHostPanelMenu_Show(pDockHostWindow, pTreeNode, ptScreen);
        break;
    }
    default:
        break;
    }

    pDockHostWindow->fDrag_ = FALSE;
    DockHostDrag_DestroyOverlay();
    if (GetCapture() == hWndDockHost)
    {
        ReleaseCapture();
    }
}

void DockHostInput_OnContextMenu(DockHostWindow* pDockHostWindow, HWND hWndContext, int x, int y)
{
    UNREFERENCED_PARAMETER(hWndContext);

    POINT pt = { x, y };
    ScreenToClient(Window_GetHWND((Window*)pDockHostWindow), &pt);

    TreeNode* pTreeNode = NULL;
    int htType = DockHostInput_HitTest(pDockHostWindow, &pTreeNode, pt.x, pt.y);
    switch (htType)
    {
    case DHT_CLOSEBTN:
    case DHT_PINBTN:
    case DHT_MOREBTN:
    case DHT_CAPTION:
        return;

    case DHT_UNKNOWN:
    {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu)
        {
            InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_DOCKINSPECTOR, L"Inspect Element...");
            TrackPopupMenu(hMenu, 0, x, y, 0, Window_GetHWND((Window*)pDockHostWindow), NULL);
        }
        return;
    }
    }
}

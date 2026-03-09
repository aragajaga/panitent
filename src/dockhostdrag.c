#include "precomp.h"

#include "dockhostdrag.h"

#include "dockhostlayout.h"
#include "dockhostmodelapply.h"
#include "docklayout.h"
#include "floatingwindowcontainer.h"
#include "win32/util.h"

#define DRAG_UNDOCK_DISTANCE 32
#define DOCK_TARGET_GUIDE_SIZE 24
#define DOCK_TARGET_GUIDE_GAP 8
#define DOCK_TARGET_GUIDE_EDGE_MARGIN 12
#define DRAG_OVERLAY_SIZE 128
#define DRAG_OVERLAY_CENTER (DRAG_OVERLAY_SIZE / 2)

static HWND g_hWndDragOverlay = NULL;

static LRESULT CALLBACK DockHostDrag_OverlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static float DockHostDrag_SmoothStep(float edge0, float edge1, float x)
{
    float t = fminf(fmaxf((x - edge0) / (edge1 - edge0), 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float DockHostDrag_ClampFloat(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

static BOOL DockHostDrag_IsAnchorCandidate(TreeNode* pNode)
{
    if (!pNode || !pNode->data)
    {
        return FALSE;
    }

    DockData* pDockData = (DockData*)pNode->data;
    if (!pDockData->hWnd || !IsWindow(pDockData->hWnd))
    {
        return FALSE;
    }

    if (pDockData->nRole == DOCK_ROLE_WORKSPACE)
    {
        return FALSE;
    }

    return DockHostLayout_NodeHasVisibleWindow(pNode);
}

static void DockHostDrag_GetGlobalTargetGuideRects(const RECT* pHostClient, RECT* pRectTop, RECT* pRectLeft, RECT* pRectRight, RECT* pRectBottom)
{
    if (!pHostClient)
    {
        return;
    }

    const int guideSize = DOCK_TARGET_GUIDE_SIZE;
    const int edgeInset = DOCK_TARGET_GUIDE_EDGE_MARGIN;
    const int cx = (pHostClient->left + pHostClient->right) / 2;
    const int cy = (pHostClient->top + pHostClient->bottom) / 2;

    if (pRectTop)
    {
        SetRect(pRectTop,
            cx - guideSize / 2,
            pHostClient->top + edgeInset,
            cx + guideSize / 2,
            pHostClient->top + edgeInset + guideSize);
    }
    if (pRectLeft)
    {
        SetRect(pRectLeft,
            pHostClient->left + edgeInset,
            cy - guideSize / 2,
            pHostClient->left + edgeInset + guideSize,
            cy + guideSize / 2);
    }
    if (pRectRight)
    {
        SetRect(pRectRight,
            pHostClient->right - edgeInset - guideSize,
            cy - guideSize / 2,
            pHostClient->right - edgeInset,
            cy + guideSize / 2);
    }
    if (pRectBottom)
    {
        SetRect(pRectBottom,
            cx - guideSize / 2,
            pHostClient->bottom - edgeInset - guideSize,
            cx + guideSize / 2,
            pHostClient->bottom - edgeInset);
    }
}

static void DockHostDrag_GetLocalTargetGuideRects(const RECT* pHostClient, const RECT* pAnchorRect, RECT* pRectCenter, RECT* pRectTop, RECT* pRectLeft, RECT* pRectRight, RECT* pRectBottom)
{
    if (!pHostClient || !pAnchorRect)
    {
        return;
    }

    const int guideSize = DOCK_TARGET_GUIDE_SIZE;
    const int guideStep = guideSize + DOCK_TARGET_GUIDE_GAP;
    int cx = (pAnchorRect->left + pAnchorRect->right) / 2;
    int cy = (pAnchorRect->top + pAnchorRect->bottom) / 2;

    cx = max(guideStep, min(cx, pHostClient->right - guideStep));
    cy = max(guideStep, min(cy, pHostClient->bottom - guideStep));

    if (pRectCenter)
    {
        SetRect(pRectCenter, cx - guideSize / 2, cy - guideSize / 2, cx + guideSize / 2, cy + guideSize / 2);
    }
    if (pRectTop)
    {
        SetRect(pRectTop, cx - guideSize / 2, cy - guideStep - guideSize / 2, cx + guideSize / 2, cy - guideStep + guideSize / 2);
    }
    if (pRectLeft)
    {
        SetRect(pRectLeft, cx - guideStep - guideSize / 2, cy - guideSize / 2, cx - guideStep + guideSize / 2, cy + guideSize / 2);
    }
    if (pRectRight)
    {
        SetRect(pRectRight, cx + guideStep - guideSize / 2, cy - guideSize / 2, cx + guideStep + guideSize / 2, cy + guideSize / 2);
    }
    if (pRectBottom)
    {
        SetRect(pRectBottom, cx - guideSize / 2, cy + guideStep - guideSize / 2, cx + guideSize / 2, cy + guideStep + guideSize / 2);
    }
}

static int DockHostDrag_HitTestGlobalTargetGuide(const RECT* pHostClient, POINT ptClient)
{
    if (!pHostClient || !PtInRect(pHostClient, ptClient))
    {
        return DKS_NONE;
    }

    RECT rcGuideTop = { 0 };
    RECT rcGuideLeft = { 0 };
    RECT rcGuideRight = { 0 };
    RECT rcGuideBottom = { 0 };
    DockHostDrag_GetGlobalTargetGuideRects(pHostClient, &rcGuideTop, &rcGuideLeft, &rcGuideRight, &rcGuideBottom);

    if (PtInRect(&rcGuideLeft, ptClient))
    {
        return DKS_LEFT;
    }
    if (PtInRect(&rcGuideRight, ptClient))
    {
        return DKS_RIGHT;
    }
    if (PtInRect(&rcGuideTop, ptClient))
    {
        return DKS_TOP;
    }
    if (PtInRect(&rcGuideBottom, ptClient))
    {
        return DKS_BOTTOM;
    }

    return DKS_NONE;
}

static int DockHostDrag_HitTestLocalTargetGuide(const RECT* pHostClient, const RECT* pAnchorRect, POINT ptClient)
{
    if (!pHostClient || !pAnchorRect)
    {
        return DKS_NONE;
    }

    RECT rcGuideCenter = { 0 };
    RECT rcGuideTop = { 0 };
    RECT rcGuideLeft = { 0 };
    RECT rcGuideRight = { 0 };
    RECT rcGuideBottom = { 0 };
    DockHostDrag_GetLocalTargetGuideRects(pHostClient, pAnchorRect, &rcGuideCenter, &rcGuideTop, &rcGuideLeft, &rcGuideRight, &rcGuideBottom);

    if (PtInRect(&rcGuideCenter, ptClient))
    {
        return DKS_CENTER;
    }
    if (PtInRect(&rcGuideLeft, ptClient))
    {
        return DKS_LEFT;
    }
    if (PtInRect(&rcGuideRight, ptClient))
    {
        return DKS_RIGHT;
    }
    if (PtInRect(&rcGuideTop, ptClient))
    {
        return DKS_TOP;
    }
    if (PtInRect(&rcGuideBottom, ptClient))
    {
        return DKS_BOTTOM;
    }

    return DKS_NONE;
}

static TreeNode* DockHostDrag_FindDockAnchorAtPoint(DockHostWindow* pDockHostWindow, POINT ptClient, RECT* pRectAnchor)
{
    if (pRectAnchor)
    {
        SetRectEmpty(pRectAnchor);
    }

    TreeNode* pRoot = DockHostWindow_GetRoot(pDockHostWindow);
    if (!pDockHostWindow || !pRoot)
    {
        return NULL;
    }

    TreeNode* pBestNode = NULL;
    RECT rcBest = { 0 };
    LONG bestArea = LONG_MAX;

    TreeTraversalRLOT traversal = { 0 };
    TreeTraversalRLOT_Init(&traversal, pRoot);

    TreeNode* pCurrent = NULL;
    while (pCurrent = TreeTraversalRLOT_GetNext(&traversal))
    {
        if (!DockHostDrag_IsAnchorCandidate(pCurrent))
        {
            continue;
        }

        DockData* pDockData = (DockData*)pCurrent->data;
        RECT rc = pDockData->rc;
        if (!PtInRect(&rc, ptClient))
        {
            continue;
        }

        LONG area = Win32_Rect_GetWidth(&rc) * Win32_Rect_GetHeight(&rc);
        if (!pBestNode || area < bestArea)
        {
            pBestNode = pCurrent;
            rcBest = rc;
            bestArea = area;
        }
    }

    TreeTraversalRLOT_Destroy(&traversal);

    if (pRectAnchor && pBestNode)
    {
        *pRectAnchor = rcBest;
    }

    return pBestNode;
}

void DockHostDrag_DestroyOverlay(void)
{
    if (g_hWndDragOverlay && IsWindow(g_hWndDragOverlay))
    {
        DestroyWindow(g_hWndDragOverlay);
    }

    g_hWndDragOverlay = NULL;
}

static void DockHostDrag_ContinueFloatingDrag(HWND hWndFloating)
{
    if (!hWndFloating || !IsWindow(hWndFloating))
    {
        return;
    }

    POINT ptCursor = { 0 };
    GetCursorPos(&ptCursor);

    ReleaseCapture();
    SetForegroundWindow(hWndFloating);
    SetActiveWindow(hWndFloating);
    SendMessage(hWndFloating, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM((short)ptCursor.x, (short)ptCursor.y));
}

void DockHostDrag_UndockToFloating(DockHostWindow* pDockHostWindow, TreeNode* pNode, int x, int y)
{
    if (!pDockHostWindow || !pNode || !pNode->data)
    {
        return;
    }

    DockData* pDockData = (DockData*)pNode->data;
    HWND hWndChild = pDockData->hWnd;
    DockPaneKind nPaneKind = pDockData->nPaneKind;
    RECT rcDockLocal = pDockData->rc;
    int dockWidth = max(Win32_Rect_GetWidth(&rcDockLocal), 1);
    int dockHeight = max(Win32_Rect_GetHeight(&rcDockLocal), 1);

    int dragOffsetX = x - rcDockLocal.left;
    int dragOffsetY = y - rcDockLocal.top;
    dragOffsetX = max(0, min(dragOffsetX, dockWidth - 1));
    dragOffsetY = max(0, min(dragOffsetY, dockHeight - 1));

    POINT ptCursor = { 0 };
    GetCursorPos(&ptCursor);

    if (!hWndChild || !IsWindow(hWndChild))
    {
        return;
    }

    if (nPaneKind == DOCK_PANE_TOOL)
    {
        if (!DockHostModelApply_RemoveToolWindow(pDockHostWindow, hWndChild, TRUE))
        {
            return;
        }
    }
    else if (nPaneKind != DOCK_PANE_DOCUMENT) {
        DockHostWindow_Undock(pDockHostWindow, pNode);
    }

    RECT rcFloating = rcDockLocal;
    MapWindowPoints(Window_GetHWND((Window*)pDockHostWindow), HWND_DESKTOP, (POINT*)&rcFloating, 2);

    int width = max(Win32_Rect_GetWidth(&rcFloating), 260);
    int height = max(Win32_Rect_GetHeight(&rcFloating), 220);
    int floatingX = ptCursor.x - dragOffsetX;
    int floatingY = ptCursor.y - dragOffsetY;
    RECT rcFloatingWindow = { floatingX, floatingY, floatingX + width, floatingY + height };

    HWND hWndFloating = NULL;
    if (nPaneKind == DOCK_PANE_DOCUMENT)
    {
        if (!DockHostModelApply_UndockDocumentWindowToFloating(
            pDockHostWindow,
            hWndChild,
            &rcFloatingWindow,
            FALSE,
            ptCursor,
            &hWndFloating))
        {
            return;
        }
    }
    else {
        FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
        hWndFloating = Window_CreateWindow((Window*)pFloatingWindowContainer, NULL);
        FloatingWindowContainer_SetDockTarget(pFloatingWindowContainer, pDockHostWindow);
        FloatingWindowContainer_SetDockPolicy(pFloatingWindowContainer, FLOAT_DOCK_POLICY_PANEL);
        FloatingWindowContainer_PinWindow(pFloatingWindowContainer, hWndChild);
        SetWindowPos(hWndFloating, NULL, floatingX, floatingY, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    DockHostWindow_Rearrange(pDockHostWindow);
    DockHostDrag_DestroyOverlay();
    DockHostDrag_ContinueFloatingDrag(hWndFloating);
}

void DockHostDrag_UpdateOverlayVisual(DockHostWindow* pDockHostWindow, int iRadius)
{
    if (!pDockHostWindow || !g_hWndDragOverlay || !IsWindow(g_hWndDragOverlay))
    {
        return;
    }

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = DRAG_OVERLAY_SIZE;
    bmi.bmiHeader.biHeight = -DRAG_OVERLAY_SIZE;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    unsigned int* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);
    memset(pBits, 0, DRAG_OVERLAY_SIZE * DRAG_OVERLAY_SIZE * sizeof(unsigned int));

    float radius = DockHostDrag_ClampFloat((float)iRadius, 0.0f, (float)(DRAG_OVERLAY_CENTER - 6));
    float readiness = DockHostDrag_ClampFloat(radius / (float)DRAG_UNDOCK_DISTANCE, 0.0f, 1.0f);
    float heat = DockHostDrag_SmoothStep(0.72f, 1.0f, readiness);
    float ringHalfWidth = 2.2f + heat * 0.8f;
    float ringFeather = 2.4f + heat * 1.2f;

    const float coldR = 0x62;
    const float coldG = 0xC6;
    const float coldB = 0xFF;
    const float hotR = 0xFF;
    const float hotG = 0x8A;
    const float hotB = 0x4A;
    float srcR = coldR + (hotR - coldR) * heat;
    float srcG = coldG + (hotG - coldG) * heat;
    float srcB = coldB + (hotB - coldB) * heat;

    for (int y = 0; y < DRAG_OVERLAY_SIZE; ++y)
    {
        for (int x = 0; x < DRAG_OVERLAY_SIZE; ++x)
        {
            float dx = (float)x + 0.5f - (float)DRAG_OVERLAY_CENTER;
            float dy = (float)y + 0.5f - (float)DRAG_OVERLAY_CENTER;
            float distance = sqrtf(dx * dx + dy * dy);
            float ringDistance = fabsf(distance - radius);
            float core = 1.0f - DockHostDrag_SmoothStep(ringHalfWidth, ringHalfWidth + ringFeather, ringDistance);
            float glow = (1.0f - DockHostDrag_SmoothStep(ringHalfWidth + 1.5f, ringHalfWidth + 8.5f, ringDistance)) * 0.38f;
            float alphaNorm = DockHostDrag_ClampFloat(core + glow, 0.0f, 1.0f);
            if (radius < 1.0f)
            {
                alphaNorm = 0.0f;
            }

            float alphaBoost = 220.0f + heat * 20.0f;
            BYTE alpha = (BYTE)DockHostDrag_ClampFloat(alphaBoost * alphaNorm, 0.0f, 255.0f);
            BYTE red = (BYTE)DockHostDrag_ClampFloat((srcR * alpha) / 255.0f, 0.0f, 255.0f);
            BYTE green = (BYTE)DockHostDrag_ClampFloat((srcG * alpha) / 255.0f, 0.0f, 255.0f);
            BYTE blue = (BYTE)DockHostDrag_ClampFloat((srcB * alpha) / 255.0f, 0.0f, 255.0f);
            pBits[y * DRAG_OVERLAY_SIZE + x] = ((unsigned int)alpha << 24) |
                ((unsigned int)red << 16) |
                ((unsigned int)green << 8) |
                (unsigned int)blue;
        }
    }

    POINT ptPos = { pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y };
    ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptPos);
    ptPos.x -= DRAG_OVERLAY_CENTER;
    ptPos.y -= DRAG_OVERLAY_CENTER;

    SIZE sizeWnd = { DRAG_OVERLAY_SIZE, DRAG_OVERLAY_SIZE };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blendFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(g_hWndDragOverlay, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blendFunction, ULW_ALPHA);

    SelectObject(hdcMem, hOldObj);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void DockHostDrag_StartDrag(DockHostWindow* pDockHostWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    WNDCLASSEX wcex = { 0 };
    if (!GetClassInfoEx(GetModuleHandle(NULL), L"__DragOverlayClass", &wcex))
    {
        wcex.cbSize = sizeof(wcex);
        wcex.lpfnWndProc = (WNDPROC)DockHostDrag_OverlayWndProc;
        wcex.hInstance = GetModuleHandle(NULL);
        wcex.lpszClassName = L"__DragOverlayClass";
        RegisterClassEx(&wcex);
    }

    POINT ptStart = { pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y };
    ClientToScreen(Window_GetHWND((Window*)pDockHostWindow), &ptStart);

    DockHostDrag_DestroyOverlay();
    g_hWndDragOverlay = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        L"__DragOverlayClass",
        L"DragOverlay",
        WS_VISIBLE | WS_POPUP,
        ptStart.x - DRAG_OVERLAY_CENTER,
        ptStart.y - DRAG_OVERLAY_CENTER,
        DRAG_OVERLAY_SIZE,
        DRAG_OVERLAY_SIZE,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    DockHostDrag_UpdateOverlayVisual(pDockHostWindow, 0);
}

BOOL DockHostDrag_HitTestDockTarget(DockHostWindow* pDockHostWindow, POINT ptScreen, DockTargetHit* pTargetHit)
{
    if (!pDockHostWindow || !pTargetHit)
    {
        return FALSE;
    }

    memset(pTargetHit, 0, sizeof(*pTargetHit));

    HWND hWndDockHost = Window_GetHWND((Window*)pDockHostWindow);
    if (!hWndDockHost || !IsWindow(hWndDockHost))
    {
        return FALSE;
    }

    RECT rcHostScreen = { 0 };
    GetWindowRect(hWndDockHost, &rcHostScreen);
    if (!PtInRect(&rcHostScreen, ptScreen))
    {
        return FALSE;
    }

    POINT ptClient = ptScreen;
    ScreenToClient(hWndDockHost, &ptClient);

    RECT rcClient = { 0 };
    GetClientRect(hWndDockHost, &rcClient);

    RECT rcAnchor = { 0 };
    TreeNode* pAnchorNode = DockHostDrag_FindDockAnchorAtPoint(pDockHostWindow, ptClient, &rcAnchor);
    if (pAnchorNode && pAnchorNode->data)
    {
        DockData* pAnchorData = (DockData*)pAnchorNode->data;
        pTargetHit->bLocalTarget = TRUE;
        pTargetHit->hWndAnchor = pAnchorData->hWnd;
        pTargetHit->rcAnchorClient = rcAnchor;

        int localSide = DockHostDrag_HitTestLocalTargetGuide(&rcClient, &rcAnchor, ptClient);
        if (localSide != DKS_NONE)
        {
            pTargetHit->nDockSide = localSide;
            if (localSide == DKS_CENTER)
            {
                pTargetHit->rcPreviewClient = rcAnchor;
            }
            else if (!DockLayout_GetDockPreviewRect(&rcAnchor, localSide, &pTargetHit->rcPreviewClient))
            {
                pTargetHit->rcPreviewClient = rcAnchor;
            }
            return TRUE;
        }
    }

    int globalSide = DockHostDrag_HitTestGlobalTargetGuide(&rcClient, ptClient);
    if (globalSide != DKS_NONE)
    {
        pTargetHit->nDockSide = globalSide;
        pTargetHit->bLocalTarget = FALSE;
        SetRectEmpty(&pTargetHit->rcAnchorClient);
        SetRectEmpty(&pTargetHit->rcPreviewClient);
        DockLayout_GetDockPreviewRect(&rcClient, globalSide, &pTargetHit->rcPreviewClient);
        return TRUE;
    }

    return TRUE;
}

int DockHostDrag_HitTestDockSide(DockHostWindow* pDockHostWindow, POINT ptScreen)
{
    DockTargetHit targetHit = { 0 };
    if (!DockHostDrag_HitTestDockTarget(pDockHostWindow, ptScreen, &targetHit))
    {
        return DKS_NONE;
    }

    return targetHit.nDockSide;
}

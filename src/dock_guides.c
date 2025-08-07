#include "precomp.h"
#include "dock_guides.h"
#include "resource.h"

// --- Private Functions ---

#define DOCK_GUIDE_CLASS_NAME L"__DockGuideWindow"

LRESULT CALLBACK DockGuide_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT)
    {
        DockGuide* pGuide = (DockGuide*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (!pGuide) return 0;

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // For simplicity, we'll just draw a colored rectangle
        // A real implementation would use UpdateLayeredWindow with a bitmap
        HBRUSH hBrush;
        if (pGuide->isHot) {
            hBrush = CreateSolidBrush(RGB(0, 120, 215)); // Blue for hot
        } else {
            hBrush = CreateSolidBrush(RGB(50, 50, 50)); // Gray for normal
        }
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL RegisterDockGuideWindowClass()
{
    WNDCLASSEXW wcex = { 0 };
    if (!GetClassInfoExW(GetModuleHandle(NULL), DOCK_GUIDE_CLASS_NAME, &wcex))
    {
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = 0;
        wcex.lpfnWndProc = DockGuide_WndProc;
        wcex.hInstance = GetModuleHandle(NULL);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.lpszClassName = DOCK_GUIDE_CLASS_NAME;
        wcex.cbWndExtra = sizeof(DockGuide*);
        if (!RegisterClassExW(&wcex))
            return FALSE;
    }
    return TRUE;
}

HWND CreateGuideWindow(DockGuide* pGuide)
{
    HWND hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        DOCK_GUIDE_CLASS_NAME,
        NULL,
        WS_POPUP,
        0, 0, 0, 0,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (hWnd) {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pGuide);
        SetLayeredWindowAttributes(hWnd, 0, 180, LWA_ALPHA);
    }
    return hWnd;
}


// --- API Implementation ---

DockGuideManager* DockGuideManager_Create()
{
    RegisterDockGuideWindowClass();
    DockGuideManager* pMgr = (DockGuideManager*)calloc(1, sizeof(DockGuideManager));
    if (!pMgr) return NULL;

    pMgr->compass.center.hWnd = CreateGuideWindow(&pMgr->compass.center);
    pMgr->compass.left.hWnd = CreateGuideWindow(&pMgr->compass.left);
    pMgr->compass.right.hWnd = CreateGuideWindow(&pMgr->compass.right);
    pMgr->compass.top.hWnd = CreateGuideWindow(&pMgr->compass.top);
    pMgr->compass.bottom.hWnd = CreateGuideWindow(&pMgr->compass.bottom);
    pMgr->siteLeft.hWnd = CreateGuideWindow(&pMgr->siteLeft);
    pMgr->siteRight.hWnd = CreateGuideWindow(&pMgr->siteRight);
    pMgr->siteTop.hWnd = CreateGuideWindow(&pMgr->siteTop);
    pMgr->siteBottom.hWnd = CreateGuideWindow(&pMgr->siteBottom);

    // Assign areas
    pMgr->compass.center.area = DOCK_DROP_AREA_CENTER;
    pMgr->compass.left.area = DOCK_DROP_AREA_LEFT;
    pMgr->compass.right.area = DOCK_DROP_AREA_RIGHT;
    pMgr->compass.top.area = DOCK_DROP_AREA_TOP;
    pMgr->compass.bottom.area = DOCK_DROP_AREA_BOTTOM;
    pMgr->siteLeft.area = DOCK_DROP_AREA_LEFT;
    pMgr->siteRight.area = DOCK_DROP_AREA_RIGHT;
    pMgr->siteTop.area = DOCK_DROP_AREA_TOP;
    pMgr->siteBottom.area = DOCK_DROP_AREA_BOTTOM;

    return pMgr;
}

void DockGuideManager_Destroy(DockGuideManager* pMgr)
{
    if (!pMgr) return;

    DestroyWindow(pMgr->compass.center.hWnd);
    DestroyWindow(pMgr->compass.left.hWnd);
    DestroyWindow(pMgr->compass.right.hWnd);
    DestroyWindow(pMgr->compass.top.hWnd);
    DestroyWindow(pMgr->compass.bottom.hWnd);
    DestroyWindow(pMgr->siteLeft.hWnd);
    DestroyWindow(pMgr->siteRight.hWnd);
    DestroyWindow(pMgr->siteTop.hWnd);
    DestroyWindow(pMgr->siteBottom.hWnd);

    free(pMgr);
}

void DockGuideManager_Show(DockGuideManager* pMgr, DockPane* pTargetPane, DockSite* pTargetSite)
{
    if (!pMgr) return;

    // For now, let's just show the site guides
    // A full implementation would show the compass over the pTargetPane
    RECT rcSite;
    GetWindowRect(pTargetSite->hWnd, &rcSite);

    int guideSize = 50;
    int siteCenterX = rcSite.left + (rcSite.right - rcSite.left) / 2;
    int siteCenterY = rcSite.top + (rcSite.bottom - rcSite.top) / 2;

    // Left
    SetWindowPos(pMgr->siteLeft.hWnd, HWND_TOPMOST, rcSite.left, siteCenterY - guideSize / 2, guideSize, guideSize, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    // Right
    SetWindowPos(pMgr->siteRight.hWnd, HWND_TOPMOST, rcSite.right - guideSize, siteCenterY - guideSize / 2, guideSize, guideSize, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    // Top
    SetWindowPos(pMgr->siteTop.hWnd, HWND_TOPMOST, siteCenterX - guideSize / 2, rcSite.top, guideSize, guideSize, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    // Bottom
    SetWindowPos(pMgr->siteBottom.hWnd, HWND_TOPMOST, siteCenterX - guideSize / 2, rcSite.bottom - guideSize, guideSize, guideSize, SWP_SHOWWINDOW | SWP_NOACTIVATE);


    pMgr->isVisible = TRUE;
}

void DockGuideManager_Hide(DockGuideManager* pMgr)
{
    if (!pMgr) return;

    ShowWindow(pMgr->compass.center.hWnd, SW_HIDE);
    ShowWindow(pMgr->compass.left.hWnd, SW_HIDE);
    ShowWindow(pMgr->compass.right.hWnd, SW_HIDE);
    ShowWindow(pMgr->compass.top.hWnd, SW_HIDE);
    ShowWindow(pMgr->compass.bottom.hWnd, SW_HIDE);
    ShowWindow(pMgr->siteLeft.hWnd, SW_HIDE);
    ShowWindow(pMgr->siteRight.hWnd, SW_HIDE);
    ShowWindow(pMgr->siteTop.hWnd, SW_HIDE);
    ShowWindow(pMgr->siteBottom.hWnd, SW_HIDE);

    pMgr->isVisible = FALSE;
}

void DockGuideManager_UpdateHighlight(DockGuideManager* pMgr, POINT screenPt)
{
    if (!pMgr || !pMgr->isVisible) return;

    // This is a simplified hit-test. A real one would check non-rectangular bitmap regions.
    DockGuide* guides[] = {
        &pMgr->compass.center, &pMgr->compass.left, &pMgr->compass.right, &pMgr->compass.top, &pMgr->compass.bottom,
        &pMgr->siteLeft, &pMgr->siteRight, &pMgr->siteTop, &pMgr->siteBottom
    };

    for (int i = 0; i < sizeof(guides) / sizeof(guides[0]); ++i) {
        DockGuide* pGuide = guides[i];
        if (!IsWindowVisible(pGuide->hWnd)) continue;

        RECT rcGuide;
        GetWindowRect(pGuide->hWnd, &rcGuide);

        BOOL isHot = PtInRect(&rcGuide, screenPt);
        if (isHot != pGuide->isHot) {
            pGuide->isHot = isHot;
            InvalidateRect(pGuide->hWnd, NULL, TRUE);
        }
    }
}

DockDropArea DockGuideManager_GetDropTarget(DockGuideManager* pMgr)
{
    if (!pMgr || !pMgr->isVisible) return DOCK_DROP_AREA_NONE;

    DockGuide* guides[] = {
        &pMgr->compass.center, &pMgr->compass.left, &pMgr->compass.right, &pMgr->compass.top, &pMgr->compass.bottom,
        &pMgr->siteLeft, &pMgr->siteRight, &pMgr->siteTop, &pMgr->siteBottom
    };

    for (int i = 0; i < sizeof(guides) / sizeof(guides[0]); ++i) {
        if (guides[i]->isHot) {
            return guides[i]->area;
        }
    }

    return DOCK_DROP_AREA_NONE;
}

#include "precomp.h"

#include "dockhostpaint.h"

#include "dockhostcaption.h"
#include "dockhostlayout.h"
#include "dockhosttree.h"
#include "dockhostzone.h"
#include "docklayout.h"
#include "panitentapp.h"
#include "resource.h"
#include "theme.h"
#include "toolwndframe.h"
#include "win32/util.h"

#define DOCKHOSTBGMARGIN 16

typedef struct DockHostWatermarkCache DockHostWatermarkCache;
struct DockHostWatermarkCache
{
    HBITMAP hSourceBitmap;
    HBITMAP hTintedBitmap;
    int width;
    int height;
    COLORREF tint;
};

static DockHostWatermarkCache g_dockHostWatermarkCache = { 0 };

static void DockHostPaint_GetPanelTabLabel(DockData* pDockData, WCHAR* pszLabel, int cchLabel)
{
    if (!pszLabel || cchLabel <= 0)
    {
        return;
    }

    pszLabel[0] = L'\0';
    if (!pDockData)
    {
        return;
    }

    if (pDockData->lpszCaption[0] != L'\0')
    {
        StringCchCopyW(pszLabel, cchLabel, pDockData->lpszCaption);
        return;
    }

    if (pDockData->lpszName[0] != L'\0')
    {
        StringCchCopyW(pszLabel, cchLabel, pDockData->lpszName);
    }
}

static int DockHostPaint_GetZoneTabLengthFromLabel(HDC hdc, PCWSTR pszLabel)
{
    SIZE size = { 0 };
    if (!hdc || !pszLabel)
    {
        return 72;
    }

    if (!GetTextExtentPoint32W(hdc, pszLabel, (int)wcslen(pszLabel), &size))
    {
        return 72;
    }

    return max(72, size.cx + 12);
}

static BOOL DockHostPaint_GetZoneTabRect(
    DockHostWindow* pDockHostWindow,
    int nDockSide,
    int iOffset,
    int iTabLength,
    int iZoneTabGutter,
    RECT* pRect)
{
    if (!pDockHostWindow || !pRect)
    {
        return FALSE;
    }

    RECT rcClient = { 0 };
    if (!DockHostWindow_GetHostContentRect(pDockHostWindow, &rcClient))
    {
        return FALSE;
    }

    return DockLayout_GetZoneTabRectByOffset(&rcClient, nDockSide, iOffset, iTabLength, pRect);
}

int DockHostPaint_HitTestZoneTab(DockHostWindow* pDockHostWindow, int x, int y, HWND* phWndTab, int iZoneTabGutter)
{
    if (phWndTab)
    {
        *phWndTab = NULL;
    }

    if (!pDockHostWindow)
    {
        return DKS_NONE;
    }

    POINT pt = { x, y };
    HDC hdc = GetDC(Window_GetHWND((Window*)pDockHostWindow));
    if (!hdc)
    {
        return DKS_NONE;
    }

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));

    const int sides[] = { DKS_LEFT, DKS_RIGHT, DKS_TOP, DKS_BOTTOM };
    for (int i = 0; i < ARRAYSIZE(sides); ++i)
    {
        TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, sides[i]);
        TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
        int nTabs = DockZone_GetPanelsByCollapsed(pZoneNode, tabs, ARRAYSIZE(tabs), TRUE);
        int iOffset = 0;

        for (int j = 0; j < nTabs; ++j)
        {
            DockData* pDockDataTab = tabs[j] ? (DockData*)tabs[j]->data : NULL;
            WCHAR szLabel[MAX_PATH] = L"";
            RECT rcTab = { 0 };
            if (!pDockDataTab || !pDockDataTab->hWnd || !IsWindow(pDockDataTab->hWnd))
            {
                continue;
            }

            DockHostPaint_GetPanelTabLabel(pDockDataTab, szLabel, ARRAYSIZE(szLabel));
            int iTabLength = DockHostPaint_GetZoneTabLengthFromLabel(hdc, szLabel);
            if (!DockHostPaint_GetZoneTabRect(pDockHostWindow, sides[i], iOffset, iTabLength, iZoneTabGutter, &rcTab))
            {
                iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;
                continue;
            }

            if (PtInRect(&rcTab, pt))
            {
                if (phWndTab)
                {
                    *phWndTab = pDockDataTab->hWnd;
                }
                ReleaseDC(Window_GetHWND((Window*)pDockHostWindow), hdc);
                return sides[i];
            }

            iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;
        }
    }

    ReleaseDC(Window_GetHWND((Window*)pDockHostWindow), hdc);
    return DKS_NONE;
}

static void DockHostPaint_DrawZoneTabs(DockHostWindow* pDockHostWindow, HDC hdc, int iZoneTabGutter)
{
    if (!pDockHostWindow || !hdc)
    {
        return;
    }

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, PanitentApp_GetUIFont(PanitentApp_Instance()));

    PanitentThemeColors colors = { 0 };
    PanitentTheme_GetColors(&colors);

    const int sides[] = { DKS_LEFT, DKS_RIGHT, DKS_TOP, DKS_BOTTOM };
    for (int i = 0; i < ARRAYSIZE(sides); ++i)
    {
        TreeNode* pZoneNode = DockHostWindow_GetZoneNode(pDockHostWindow, sides[i]);
        TreeNode* tabs[DOCK_ZONE_MAX_TABS] = { 0 };
        int nTabs = DockZone_GetPanelsByCollapsed(pZoneNode, tabs, ARRAYSIZE(tabs), TRUE);
        int iOffset = 0;

        for (int j = 0; j < nTabs; ++j)
        {
            DockData* pDockDataTab = tabs[j] ? (DockData*)tabs[j]->data : NULL;
            WCHAR szLabel[MAX_PATH] = L"";
            RECT rcTab = { 0 };
            if (!pDockDataTab || !pDockDataTab->hWnd || !IsWindow(pDockDataTab->hWnd))
            {
                continue;
            }

            DockHostPaint_GetPanelTabLabel(pDockDataTab, szLabel, ARRAYSIZE(szLabel));
            int iTabLength = DockHostPaint_GetZoneTabLengthFromLabel(hdc, szLabel);
            if (!DockHostPaint_GetZoneTabRect(pDockHostWindow, sides[i], iOffset, iTabLength, iZoneTabGutter, &rcTab))
            {
                iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;
                continue;
            }

            HBRUSH hBrush = CreateSolidBrush(colors.workspaceHeader);
            FillRect(hdc, &rcTab, hBrush);
            DeleteObject(hBrush);

            HPEN hPen = CreatePen(PS_SOLID, 1, colors.border);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rcTab.left, rcTab.top, rcTab.right, rcTab.bottom);
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            SetTextColor(hdc, colors.text);
            DrawTextW(
                hdc,
                szLabel,
                -1,
                &rcTab,
                DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

            iOffset += iTabLength + DOCKLAYOUT_ZONE_TAB_GAP;
        }
    }
}

static BOOL DockHostPaint_EnsureWatermarkCache(HDC hdc, int idBitmap, COLORREF tint)
{
    HDC hdcMask = NULL;
    HBITMAP hOldBitmap = NULL;
    BITMAP bm = { 0 };
    BITMAPINFO bmi = { 0 };
    uint32_t* pPixels = NULL;
    HDC hdcColored = NULL;
    HBITMAP hOldColored = NULL;
    BYTE targetR = GetRValue(tint);
    BYTE targetG = GetGValue(tint);
    BYTE targetB = GetBValue(tint);

    if (!hdc)
    {
        return FALSE;
    }

    if (!g_dockHostWatermarkCache.hSourceBitmap)
    {
        g_dockHostWatermarkCache.hSourceBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(idBitmap));
        if (!g_dockHostWatermarkCache.hSourceBitmap)
        {
            return FALSE;
        }

        if (!GetObject(g_dockHostWatermarkCache.hSourceBitmap, sizeof(BITMAP), &bm))
        {
            DeleteObject(g_dockHostWatermarkCache.hSourceBitmap);
            g_dockHostWatermarkCache.hSourceBitmap = NULL;
            return FALSE;
        }

        g_dockHostWatermarkCache.width = bm.bmWidth;
        g_dockHostWatermarkCache.height = bm.bmHeight;
    }

    if (g_dockHostWatermarkCache.hTintedBitmap &&
        g_dockHostWatermarkCache.tint == tint)
    {
        return TRUE;
    }

    if (g_dockHostWatermarkCache.hTintedBitmap)
    {
        DeleteObject(g_dockHostWatermarkCache.hTintedBitmap);
        g_dockHostWatermarkCache.hTintedBitmap = NULL;
    }

    if (g_dockHostWatermarkCache.width <= 0 || g_dockHostWatermarkCache.height <= 0)
    {
        return FALSE;
    }

    hdcMask = CreateCompatibleDC(hdc);
    if (!hdcMask)
    {
        return FALSE;
    }

    hOldBitmap = (HBITMAP)SelectObject(hdcMask, g_dockHostWatermarkCache.hSourceBitmap);
    if (!hOldBitmap)
    {
        DeleteDC(hdcMask);
        return FALSE;
    }

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_dockHostWatermarkCache.width;
    bmi.bmiHeader.biHeight = -g_dockHostWatermarkCache.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_dockHostWatermarkCache.hTintedBitmap = CreateDIBSection(
        hdc,
        &bmi,
        DIB_RGB_COLORS,
        (LPVOID*)&pPixels,
        NULL,
        0);
    if (!g_dockHostWatermarkCache.hTintedBitmap || !pPixels)
    {
        g_dockHostWatermarkCache.hTintedBitmap = NULL;
        SelectObject(hdcMask, hOldBitmap);
        DeleteDC(hdcMask);
        return FALSE;
    }

    hdcColored = CreateCompatibleDC(hdc);
    if (!hdcColored)
    {
        DeleteObject(g_dockHostWatermarkCache.hTintedBitmap);
        g_dockHostWatermarkCache.hTintedBitmap = NULL;
        SelectObject(hdcMask, hOldBitmap);
        DeleteDC(hdcMask);
        return FALSE;
    }

    hOldColored = (HBITMAP)SelectObject(hdcColored, g_dockHostWatermarkCache.hTintedBitmap);
    if (!hOldColored)
    {
        DeleteDC(hdcColored);
        DeleteObject(g_dockHostWatermarkCache.hTintedBitmap);
        g_dockHostWatermarkCache.hTintedBitmap = NULL;
        SelectObject(hdcMask, hOldBitmap);
        DeleteDC(hdcMask);
        return FALSE;
    }

    for (int y = 0; y < g_dockHostWatermarkCache.height; ++y)
    {
        for (int x = 0; x < g_dockHostWatermarkCache.width; ++x)
        {
            COLORREF srcColor = GetPixel(hdcMask, x, y);
            BYTE mask = (BYTE)(((int)GetRValue(srcColor) + (int)GetGValue(srcColor) + (int)GetBValue(srcColor)) / 3);
            BYTE outR = (BYTE)(((int)targetR * (int)mask + 127) / 255);
            BYTE outG = (BYTE)(((int)targetG * (int)mask + 127) / 255);
            BYTE outB = (BYTE)(((int)targetB * (int)mask + 127) / 255);

            pPixels[(size_t)y * (size_t)g_dockHostWatermarkCache.width + (size_t)x] =
                ((uint32_t)mask << 24) |
                ((uint32_t)outR << 16) |
                ((uint32_t)outG << 8) |
                (uint32_t)outB;
        }
    }

    g_dockHostWatermarkCache.tint = tint;

    SelectObject(hdcColored, hOldColored);
    DeleteDC(hdcColored);
    SelectObject(hdcMask, hOldBitmap);
    DeleteDC(hdcMask);
    return TRUE;
}

static BOOL DockHostPaint_DrawMaskedBitmap(HDC hdc, const RECT* pDestRect, int idBitmap, COLORREF tint)
{
    HDC hdcColored = NULL;
    HBITMAP hOldColored = NULL;
    int destWidth = 0;
    int destHeight = 0;
    BOOL fDrawn = FALSE;

    if (!hdc || !pDestRect)
    {
        return FALSE;
    }

    destWidth = pDestRect->right - pDestRect->left;
    destHeight = pDestRect->bottom - pDestRect->top;
    if (destWidth <= 0 || destHeight <= 0)
    {
        return FALSE;
    }

    if (!DockHostPaint_EnsureWatermarkCache(hdc, idBitmap, tint))
    {
        return FALSE;
    }

    hdcColored = CreateCompatibleDC(hdc);
    if (!hdcColored)
    {
        return FALSE;
    }

    hOldColored = (HBITMAP)SelectObject(hdcColored, g_dockHostWatermarkCache.hTintedBitmap);
    if (!hOldColored)
    {
        DeleteDC(hdcColored);
        return FALSE;
    }

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 0xFF;
    blend.AlphaFormat = AC_SRC_ALPHA;
    fDrawn = AlphaBlend(
        hdc,
        pDestRect->left,
        pDestRect->top,
        destWidth,
        destHeight,
        hdcColored,
        0,
        0,
        g_dockHostWatermarkCache.width,
        g_dockHostWatermarkCache.height,
        blend);

    SelectObject(hdcColored, hOldColored);
    DeleteDC(hdcColored);
    return fDrawn;
}

static void DockHostPaint_DrawSplitGrips(DockHostWindow* pDockHostWindow, HDC hdc, int iBorderWidth)
{
    if (!pDockHostWindow || !DockHostWindow_GetRoot(pDockHostWindow))
    {
        return;
    }

    TreeTraversalRLOT traversal = { 0 };
    TreeTraversalRLOT_Init(&traversal, DockHostWindow_GetRoot(pDockHostWindow));

    TreeNode* pCurrentNode = NULL;
    while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
    {
        RECT rcSplit = { 0 };
        if (!DockHostLayout_GetSplitRect(pCurrentNode, &rcSplit, iBorderWidth))
        {
            continue;
        }

        BOOL bVertical = DockHostLayout_IsSplitVertical(pCurrentNode);
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SelectObject(hdc, GetStockObject(DC_PEN));

        PanitentThemeColors colors = { 0 };
        PanitentTheme_GetColors(&colors);
        SetDCBrushColor(hdc, colors.splitterFill);
        SetDCPenColor(hdc, colors.border);
        Rectangle(hdc, rcSplit.left, rcSplit.top, rcSplit.right, rcSplit.bottom);

        SetDCBrushColor(hdc, colors.splitterGrip);
        if (bVertical)
        {
            int cx = (rcSplit.left + rcSplit.right) / 2;
            int cy = (rcSplit.top + rcSplit.bottom) / 2;
            for (int i = -1; i <= 1; ++i)
            {
                Rectangle(hdc, cx - 2 + i * 8, cy - 1, cx + 2 + i * 8, cy + 1);
            }
        }
        else {
            int cx = (rcSplit.left + rcSplit.right) / 2;
            int cy = (rcSplit.top + rcSplit.bottom) / 2;
            for (int i = -1; i <= 1; ++i)
            {
                Rectangle(hdc, cx - 1, cy - 2 + i * 8, cx + 1, cy + 2 + i * 8);
            }
        }
    }

    TreeTraversalRLOT_Destroy(&traversal);
}

void DockHostPaint_PaintContent(
    DockHostWindow* pDockHostWindow,
    HDC hdc,
    const RECT* pClientRect,
    HBRUSH hCaptionBrush,
    int iZoneTabGutter,
    int iBorderWidth)
{
    if (!pDockHostWindow || !hdc || !pClientRect)
    {
        return;
    }

    SelectObject(hdc, GetStockObject(DC_BRUSH));
    PanitentThemeColors colors = { 0 };
    PanitentTheme_GetColors(&colors);
    SetDCBrushColor(hdc, colors.rootBackground);
    FillRect(hdc, pClientRect, (HBRUSH)GetStockObject(DC_BRUSH));

    if (DockHostWindow_GetRoot(pDockHostWindow))
    {
        DockNode_Paint(pDockHostWindow, DockHostWindow_GetRoot(pDockHostWindow), hdc, hCaptionBrush);
        DockHostPaint_DrawSplitGrips(pDockHostWindow, hdc, iBorderWidth);
        DockHostPaint_DrawZoneTabs(pDockHostWindow, hdc, iZoneTabGutter);
    }
    else {
        HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCKHOSTBG));
        if (hBitmap)
        {
            BITMAP bm = { 0 };
            GetObject(hBitmap, sizeof(BITMAP), &bm);
            if (bm.bmWidth > 0 && bm.bmHeight > 0)
            {
                RECT rcLogo = { 0 };
                int clientWidth = pClientRect->right - pClientRect->left;
                int clientHeight = pClientRect->bottom - pClientRect->top;
                SetRect(
                    &rcLogo,
                    clientWidth - bm.bmWidth - DOCKHOSTBGMARGIN,
                    clientHeight - bm.bmHeight - DOCKHOSTBGMARGIN,
                    clientWidth - DOCKHOSTBGMARGIN,
                    clientHeight - DOCKHOSTBGMARGIN);
                DockHostPaint_DrawMaskedBitmap(hdc, &rcLogo, IDB_DOCKHOSTBG, colors.accentActive);
            }
            DeleteObject(hBitmap);
        }
    }
}
